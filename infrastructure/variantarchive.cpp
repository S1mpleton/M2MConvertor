#include "infrastructure/variantarchive.h"

#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <openssl/evp.h>

#include <algorithm>
#include <memory>

namespace {
const QByteArray kMagic("M2MLITE1");
constexpr int kSaltSize = 16;
constexpr int kNonceSize = 12;
constexpr int kTagSize = 16;
constexpr int kKeySize = 32;
constexpr int kPbkdf2Iterations = 200000;
using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

QByteArray applicationArchiveKeyMaterial() {
    static const QByteArray partA = QByteArray::fromHex("e758d0ffb756c99a9fed62dbb728a1e4"
                                                        "cb06939e6d0d017dbb321b89a262c6e8");
    static const QByteArray partB = QByteArray::fromHex("91ed552426a3a7bc31a8c75f11d2ec96"
                                                        "aec327628668c5aa074e99718add5531");
    QByteArray material(kKeySize, Qt::Uninitialized);
    for (int i = 0; i < material.size(); ++i)
        material[i] = partA[i] ^ partB[i];
    return material;
}

bool deriveKey(const QByteArray& salt, QByteArray* key) {
    if (!key)
        return false;
    key->resize(kKeySize);
    const QByteArray material = applicationArchiveKeyMaterial();
    return PKCS5_PBKDF2_HMAC(material.constData(), material.size(),
                             reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
                             kPbkdf2Iterations, EVP_sha256(), key->size(),
                             reinterpret_cast<unsigned char*>(key->data())) == 1;
}

bool decrypt(const QByteArray& encrypted, const QByteArray& salt, const QByteArray& nonce,
             const QByteArray& tag, QByteArray* plain) {
    QByteArray key;
    if (!plain || !deriveKey(salt, &key))
        return false;
    CipherContext context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context)
        return false;
    if (EVP_DecryptInit_ex(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) != 1 ||
        EVP_DecryptInit_ex(context.get(), nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(nonce.constData())) != 1)
        return false;
    int written = 0;
    if (EVP_DecryptUpdate(context.get(), nullptr, &written,
                          reinterpret_cast<const unsigned char*>(kMagic.constData()),
                          kMagic.size()) != 1)
        return false;
    plain->resize(encrypted.size());
    int total = 0;
    if (EVP_DecryptUpdate(context.get(), reinterpret_cast<unsigned char*>(plain->data()), &written,
                          reinterpret_cast<const unsigned char*>(encrypted.constData()),
                          encrypted.size()) != 1)
        return false;
    total = written;
    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_TAG, tag.size(),
                            const_cast<char*>(tag.constData())) != 1 ||
        EVP_DecryptFinal_ex(context.get(), reinterpret_cast<unsigned char*>(plain->data()) + total,
                            &written) != 1)
        return false;
    plain->resize(total + written);
    return true;
}

bool stringList(const QJsonValue& value, QStringList* result) {
    if (!result || !value.isArray())
        return false;
    result->clear();
    for (const QJsonValue& item : value.toArray()) {
        if (!item.isString())
            return false;
        result->append(item.toString());
    }
    return true;
}

bool parseVariant(const QJsonObject& object, TrainingVariant* variant, Message* error) {
    auto fail = [error](const Message& message) {
        if (error)
            *error = message;
        return false;
    };
    TrainingVariant parsed;
    parsed.number = object.value(QStringLiteral("number")).toInt(0);
    const QString type = object.value(QStringLiteral("type")).toString();
    if (parsed.number <= 0 ||
        (type != QStringLiteral("MealyToMoore") && type != QStringLiteral("MooreToMealy")))
        return fail(Message(
            QT_TRANSLATE_NOOP("Result", "Архив содержит неверный номер или тип варианта.")));
    parsed.direction = type == QStringLiteral("MealyToMoore") ? ConversionDirection::MealyToMoore
                                                              : ConversionDirection::MooreToMealy;
    auto readCounts = [](const QJsonValue& value, AutomatonCounts* counts) {
        if (!value.isObject())
            return false;
        const auto obj = value.toObject();
        counts->states = obj.value("states").toInt(-1);
        counts->inputs = obj.value("inputs").toInt(-1);
        counts->outputs = obj.value("outputs").toInt(-1);
        counts->transitions = obj.contains("transitions") ? obj.value("transitions").toInt(-1) : 1;
        return counts->states >= 1 && counts->states <= 999999 && counts->inputs >= 1 &&
               counts->inputs <= 10 && counts->outputs >= 1 && counts->outputs <= 10 &&
               counts->transitions >= 1 && counts->transitions <= 999999;
    };
    if (parsed.number > 999999 || !readCounts(object.value("mealy"), &parsed.mealy) ||
        !readCounts(object.value("moore"), &parsed.moore))
        return fail(Message(QT_TRANSLATE_NOOP(
            "Result", "В варианте повреждены количества состояний или сигналов.")));
    if (!object.value("reaction").isArray() || object.value("reaction").toArray().isEmpty())
        return fail(
            Message(QT_TRANSLATE_NOOP("Result", "В варианте отсутствует реакция автомата.")));
    const QRegularExpression inputPattern(QStringLiteral("^x[0-9]$"));
    const QRegularExpression outputPattern(QStringLiteral("^y[0-9]$"));
    QSet<QString> inputs, outputs;
    const auto steps = object.value("reaction").toArray();
    for (int i = 0; i < steps.size(); ++i) {
        const auto step = steps[i].toObject();
        ReactionStep reaction;
        reaction.input = step.value("input").toString();
        const bool terminalMooreOutput = parsed.direction == ConversionDirection::MooreToMealy &&
            i + 1 == steps.size() && reaction.input.isEmpty();
        if (!step.value("input").isString() ||
            (!terminalMooreOutput && !inputPattern.match(reaction.input).hasMatch()) ||
            !stringList(step.value("outputs"), &reaction.outputs))
            return fail(Message(QT_TRANSLATE_NOOP("Result", "Повреждён такт реакции автомата.")));
        QSet<QString> seen;
        for (const auto& output : reaction.outputs) {
            if (!outputPattern.match(output).hasMatch() || seen.contains(output))
                return fail(Message(QT_TRANSLATE_NOOP(
                    "Result", "Неверные или повторяющиеся выходы в такте реакции.")));
            seen.insert(output);
            outputs.insert(output);
        }
        if (!reaction.input.isEmpty())
            inputs.insert(reaction.input);
        parsed.reaction.append(reaction);
    }
    const auto& source =
        parsed.direction == ConversionDirection::MooreToMealy ? parsed.moore : parsed.mealy;
    if (inputs.size() != source.inputs || outputs.size() != source.outputs)
        return fail(Message(QT_TRANSLATE_NOOP(
                                "Result", "В варианте №%1 количества сигналов исходного автомата "
                                          "не совпадают с реакцией. Исправьте вариант в M2MTools."))
                        .arg(parsed.number));
    if (object.contains("image")) {
        const auto image = object.value("image").toObject();
        const auto encoded = image.value("data").toString();
        if (encoded.size() > ((10 * 1024 * 1024 + 2) / 3) * 4)
            return fail(
                Message(QT_TRANSLATE_NOOP("Result", "Изображение задания превышает 10 МБ.")));
        const auto decoded = QByteArray::fromBase64Encoding(
            encoded.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || decoded.decoded.isEmpty() || decoded.decoded.size() > 10 * 1024 * 1024 ||
            QImage::fromData(decoded.decoded).isNull() ||
            image.value("fileName").toString().trimmed().isEmpty())
            return fail(Message(QT_TRANSLATE_NOOP("Result", "Изображение задания повреждено.")));
        parsed.diagram = decoded.decoded;
        parsed.diagramFileName = image.value("fileName").toString();
    }
    *variant = parsed;
    return true;
}

bool readDatabase(const QString& path, QList<TrainingVariant>* variants, Message* error) {
    const QString connection = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const bool ok = [&] {
        auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        database.setDatabaseName(path);
        auto fail = [&](const QString& details) {
            *error = Message(QT_TRANSLATE_NOOP("Result", "Не удалось прочитать базу вариантов: %1")).arg(details);
            return false;
        };
        if (!database.open() || !database.transaction()) return fail(database.lastError().text());
        QSqlQuery metadata(database);
        if (!metadata.exec("SELECT version FROM LiteMetadata") || !metadata.next() ||
            metadata.value(0).toInt() != 3 || metadata.next()) {
            *error = Message(QT_TRANSLATE_NOOP("Result", "Нужна database.db новой структуры M2MTools с количеством переходов."));
            return false;
        }
        QSqlQuery rows(database);
        if (!rows.exec("SELECT id,number,conversion,mealy_states,mealy_inputs,mealy_outputs,mealy_transitions,"
                       "moore_states,moore_inputs,moore_outputs,moore_transitions FROM Variants ORDER BY number"))
            return fail(rows.lastError().text());
        QList<TrainingVariant> loaded;
        QSet<int> numbers;
        while (rows.next()) {
            const int id = rows.value(0).toInt();
            auto counts = [&](int start) {
                return QJsonObject{{"states", rows.value(start).toInt()}, {"inputs", rows.value(start + 1).toInt()},
                    {"outputs", rows.value(start + 2).toInt()}, {"transitions", rows.value(start + 3).toInt()}};
            };
            QJsonObject object{{"number", rows.value(1).toInt()}, {"type", rows.value(2).toString()},
                {"mealy", counts(3)}, {"moore", counts(7)}};
            QSqlQuery steps(database);
            steps.prepare("SELECT position,input,outputs_json FROM ReactionSteps WHERE variant_id=:id ORDER BY position");
            steps.bindValue(":id", id);
            if (!steps.exec()) return fail(steps.lastError().text());
            QJsonArray reaction;
            while (steps.next()) {
                const auto outputs = QJsonDocument::fromJson(steps.value(2).toString().toUtf8());
                if (steps.value(0).toInt() != reaction.size() || !outputs.isArray()) {
                    *error = Message(QT_TRANSLATE_NOOP("Result", "Повреждён порядок тактов или выходы реакции в базе."));
                    return false;
                }
                reaction.append(QJsonObject{{"input", steps.value(1).toString()}, {"outputs", outputs.array()}});
            }
            object.insert("reaction", reaction);
            QSqlQuery image(database);
            image.prepare("SELECT data,file_name FROM VariantImages WHERE variant_id=:id");
            image.bindValue(":id", id);
            if (!image.exec()) return fail(image.lastError().text());
            if (image.next())
                object.insert("image", QJsonObject{{"fileName", image.value(1).toString()},
                    {"data", QString::fromLatin1(image.value(0).toByteArray().toBase64())}});
            TrainingVariant variant;
            if (!parseVariant(object, &variant, error)) return false;
            if (numbers.contains(variant.number)) {
                *error = Message(QT_TRANSLATE_NOOP("Result", "В BazaVar повторяется вариант №%1.")).arg(variant.number);
                return false;
            }
            numbers.insert(variant.number);
            loaded.append(variant);
        }
        *variants = loaded;
        return true;
    }();
    QSqlDatabase::removeDatabase(connection);
    return ok;
}
} // namespace

QString conversionDirectionName(ConversionDirection direction) {
    return direction == ConversionDirection::MealyToMoore
               ? QCoreApplication::translate("Result", "Мили → Мура")
               : QCoreApplication::translate("Result", "Мура → Мили");
}

Result VariantArchive::read(const QString& path, QList<TrainingVariant>* variants) {
    Message message;
    auto* error = &message;
    auto operation = [&]() -> bool {
        if (!variants)
            return false;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP("Result", "Не удалось открыть BazaVar: %1"))
                             .arg(file.errorString());
            return false;
        }
        if (file.peek(16).startsWith("SQLite format 3")) {
            file.close();
            return readDatabase(path, variants, error);
        }
        const QByteArray archive = file.readAll();
        const int prefixSize = kMagic.size() + kSaltSize + kNonceSize + kTagSize;
        if (archive.size() <= prefixSize || !archive.startsWith(kMagic)) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP(
                    "Result", "Нужен новый архив .m2m с реакциями из M2MTools (ветка Lite). Старые "
                              "базы с автоматами не поддерживаются."));
            return false;
        }
        int offset = kMagic.size();
        const QByteArray salt = archive.mid(offset, kSaltSize);
        offset += kSaltSize;
        const QByteArray nonce = archive.mid(offset, kNonceSize);
        offset += kNonceSize;
        const QByteArray tag = archive.mid(offset, kTagSize);
        offset += kTagSize;
        QByteArray plain;
        if (!decrypt(archive.mid(offset), salt, nonce, tag, &plain)) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP(
                    "Result", "BazaVar повреждён либо создан несовместимой версией M2MTools."));
            return false;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(plain, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            if (error)
                *error = Message(
                    QT_TRANSLATE_NOOP("Result", "Расшифрованные данные BazaVar повреждены."));
            return false;
        }
        const QJsonObject root = document.object();
        if (root.value(QStringLiteral("format")).toString() != QStringLiteral("M2MToolsLite") ||
            root.value(QStringLiteral("version")).toInt() != 1 ||
            !root.value(QStringLiteral("variants")).isArray()) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP("Result", "Версия BazaVar не поддерживается."));
            return false;
        }
        QList<TrainingVariant> loaded;
        QSet<int> numbers;
        for (const QJsonValue& value : root.value(QStringLiteral("variants")).toArray()) {
            if (!value.isObject()) {
                if (error)
                    *error = Message(
                        QT_TRANSLATE_NOOP("Result", "Архив содержит повреждённый вариант."));
                return false;
            }
            TrainingVariant variant;
            if (!parseVariant(value.toObject(), &variant, error))
                return false;
            if (numbers.contains(variant.number)) {
                if (error)
                    *error =
                        Message(QT_TRANSLATE_NOOP("Result", "В BazaVar повторяется вариант №%1."))
                            .arg(variant.number);
                return false;
            }
            numbers.insert(variant.number);
            loaded.append(variant);
        }
        std::sort(loaded.begin(), loaded.end(),
                  [](const auto& left, const auto& right) { return left.number < right.number; });
        *variants = loaded;
        return true;
    };
    if (operation())
        return Result::success(
            Message(QT_TRANSLATE_NOOP("Result", "Загружено вариантов: %1.")).arg(variants->size()),
            ResultCategory::Archive);
    return Result::error(message.isEmpty()
                             ? Message(QT_TRANSLATE_NOOP("Result", "Не удалось прочитать архив."))
                             : message,
                         ResultCategory::Archive)
        .withDetails(path);
}
bool VariantArchive::load(const QString& path, QList<TrainingVariant>* variants, QString* error) {
    const auto result = read(path, variants);
    if (error && !result.ok())
        *error = result.text();
    return result.ok();
}
