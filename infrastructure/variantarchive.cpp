#include "infrastructure/variantarchive.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QRegularExpression>
#include <QImage>

#include <openssl/evp.h>

#include <memory>
#include <algorithm>

namespace {
const QByteArray kMagic("M2MLITE1");
constexpr int kSaltSize = 16;
constexpr int kNonceSize = 12;
constexpr int kTagSize = 16;
constexpr int kKeySize = 32;
constexpr int kPbkdf2Iterations = 200000;
using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

QByteArray applicationArchiveKeyMaterial() {
    static const QByteArray partA = QByteArray::fromHex(
        "e758d0ffb756c99a9fed62dbb728a1e4"
        "cb06939e6d0d017dbb321b89a262c6e8");
    static const QByteArray partB = QByteArray::fromHex(
        "91ed552426a3a7bc31a8c75f11d2ec96"
        "aec327628668c5aa074e99718add5531");
    QByteArray material(kKeySize, Qt::Uninitialized);
    for (int i = 0; i < material.size(); ++i) material[i] = partA[i] ^ partB[i];
    return material;
}

bool deriveKey(const QByteArray& salt, QByteArray* key) {
    if (!key) return false;
    key->resize(kKeySize);
    const QByteArray material = applicationArchiveKeyMaterial();
    return PKCS5_PBKDF2_HMAC(
        material.constData(), material.size(),
        reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
        kPbkdf2Iterations, EVP_sha256(), key->size(),
        reinterpret_cast<unsigned char*>(key->data())) == 1;
}

bool decrypt(const QByteArray& encrypted, const QByteArray& salt,
             const QByteArray& nonce, const QByteArray& tag, QByteArray* plain) {
    QByteArray key;
    if (!plain || !deriveKey(salt, &key)) return false;
    CipherContext context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context) return false;
    if (EVP_DecryptInit_ex(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) != 1 ||
        EVP_DecryptInit_ex(context.get(), nullptr, nullptr,
            reinterpret_cast<const unsigned char*>(key.constData()),
            reinterpret_cast<const unsigned char*>(nonce.constData())) != 1)
        return false;
    int written = 0;
    if (EVP_DecryptUpdate(context.get(), nullptr, &written,
            reinterpret_cast<const unsigned char*>(kMagic.constData()), kMagic.size()) != 1)
        return false;
    plain->resize(encrypted.size());
    int total = 0;
    if (EVP_DecryptUpdate(context.get(),
            reinterpret_cast<unsigned char*>(plain->data()), &written,
            reinterpret_cast<const unsigned char*>(encrypted.constData()), encrypted.size()) != 1)
        return false;
    total = written;
    if (EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_TAG, tag.size(),
                            const_cast<char*>(tag.constData())) != 1 ||
        EVP_DecryptFinal_ex(context.get(),
            reinterpret_cast<unsigned char*>(plain->data()) + total, &written) != 1)
        return false;
    plain->resize(total + written);
    return true;
}

bool stringList(const QJsonValue& value, QStringList* result) {
    if (!result || !value.isArray()) return false;
    result->clear();
    for (const QJsonValue& item : value.toArray()) {
        if (!item.isString()) return false;
        result->append(item.toString());
    }
    return true;
}

bool parseVariant(const QJsonObject& object, TrainingVariant* variant,
                  QString* error) {
    auto fail = [error](const QString& message) {
        if (error) *error = message;
        return false;
    };
    TrainingVariant parsed;
    parsed.number = object.value(QStringLiteral("number")).toInt(0);
    const QString type = object.value(QStringLiteral("type")).toString();
    if (parsed.number <= 0 ||
        (type != QStringLiteral("MealyToMoore") && type != QStringLiteral("MooreToMealy")))
        return fail(QStringLiteral("Архив содержит неверный номер или тип варианта."));
    parsed.direction = type == QStringLiteral("MealyToMoore")
        ? ConversionDirection::MealyToMoore : ConversionDirection::MooreToMealy;
    auto readCounts = [](const QJsonValue& value, AutomatonCounts* counts) {
        if (!value.isObject()) return false;
        const auto obj = value.toObject();
        counts->states = obj.value("states").toInt(-1);
        counts->inputs = obj.value("inputs").toInt(-1);
        counts->outputs = obj.value("outputs").toInt(-1);
        return counts->states >= 1 && counts->states <= 999999 &&
               counts->inputs >= 1 && counts->inputs <= 10 &&
               counts->outputs >= 1 && counts->outputs <= 10;
    };
    if (parsed.number > 999999 || !readCounts(object.value("mealy"), &parsed.mealy) ||
        !readCounts(object.value("moore"), &parsed.moore))
        return fail(QStringLiteral("В варианте повреждены количества состояний или сигналов."));
    if (!object.value("reaction").isArray() || object.value("reaction").toArray().isEmpty())
        return fail(QStringLiteral("В варианте отсутствует реакция автомата."));
    const QRegularExpression inputPattern(QStringLiteral("^x[0-9]$"));
    const QRegularExpression outputPattern(QStringLiteral("^y[0-9]$"));
    QSet<QString> inputs, outputs;
    for (const auto& value : object.value("reaction").toArray()) {
        const auto step = value.toObject();
        ReactionStep reaction;
        reaction.input = step.value("input").toString();
        if (!inputPattern.match(reaction.input).hasMatch() ||
            !stringList(step.value("outputs"), &reaction.outputs))
            return fail(QStringLiteral("Повреждён такт реакции автомата."));
        QSet<QString> seen;
        for (const auto& output : reaction.outputs) {
            if (!outputPattern.match(output).hasMatch() || seen.contains(output))
                return fail(QStringLiteral("Неверные или повторяющиеся выходы в такте реакции."));
            seen.insert(output);
            outputs.insert(output);
        }
        inputs.insert(reaction.input);
        parsed.reaction.append(reaction);
    }
    const auto& source = parsed.direction == ConversionDirection::MooreToMealy
                             ? parsed.moore : parsed.mealy;
    if (inputs.size() != source.inputs || outputs.size() != source.outputs)
        return fail(QStringLiteral("В варианте №%1 количества сигналов исходного автомата "
                                   "не совпадают с реакцией. Исправьте вариант в M2MTools.")
                        .arg(parsed.number));
    if (object.contains("image")) {
        const auto image = object.value("image").toObject();
        const auto encoded = image.value("data").toString();
        if (encoded.size() > ((10 * 1024 * 1024 + 2) / 3) * 4)
            return fail(QStringLiteral("Изображение задания превышает 10 МБ."));
        const auto decoded = QByteArray::fromBase64Encoding(
            encoded.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded || decoded.decoded.isEmpty() ||
            decoded.decoded.size() > 10 * 1024 * 1024 ||
            QImage::fromData(decoded.decoded).isNull() ||
            image.value("fileName").toString().trimmed().isEmpty())
            return fail(QStringLiteral("Изображение задания повреждено."));
        parsed.diagram = decoded.decoded;
        parsed.diagramFileName = image.value("fileName").toString();
    }
    *variant = parsed;
    return true;
}
}

QString conversionDirectionName(ConversionDirection direction) {
    return direction == ConversionDirection::MealyToMoore
        ? QStringLiteral("Мили → Мура") : QStringLiteral("Мура → Мили");
}

bool VariantArchive::load(const QString& path, QList<TrainingVariant>* variants,
                          QString* error) {
    if (!variants) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Не удалось открыть BazaVar: %1").arg(file.errorString());
        return false;
    }
    const QByteArray archive = file.readAll();
    const int prefixSize = kMagic.size() + kSaltSize + kNonceSize + kTagSize;
    if (archive.size() <= prefixSize || !archive.startsWith(kMagic)) {
        if (error) *error = QStringLiteral("Нужен новый архив .m2m с реакциями из M2MTools (ветка Lite). Старые базы с автоматами не поддерживаются.");
        return false;
    }
    int offset = kMagic.size();
    const QByteArray salt = archive.mid(offset, kSaltSize); offset += kSaltSize;
    const QByteArray nonce = archive.mid(offset, kNonceSize); offset += kNonceSize;
    const QByteArray tag = archive.mid(offset, kTagSize); offset += kTagSize;
    QByteArray plain;
    if (!decrypt(archive.mid(offset), salt, nonce, tag, &plain)) {
        if (error) *error = QStringLiteral(
            "BazaVar повреждён либо создан несовместимой версией M2MTools.");
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(plain, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("Расшифрованные данные BazaVar повреждены.");
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("M2MToolsLite") ||
        root.value(QStringLiteral("version")).toInt() != 1 ||
        !root.value(QStringLiteral("variants")).isArray()) {
        if (error) *error = QStringLiteral("Версия BazaVar не поддерживается.");
        return false;
    }
    QList<TrainingVariant> loaded;
    QSet<int> numbers;
    for (const QJsonValue& value : root.value(QStringLiteral("variants")).toArray()) {
        if (!value.isObject()) {
            if (error) *error = QStringLiteral("Архив содержит повреждённый вариант.");
            return false;
        }
        TrainingVariant variant;
        if (!parseVariant(value.toObject(), &variant, error)) return false;
        if (numbers.contains(variant.number)) {
            if (error) *error = QStringLiteral("В BazaVar повторяется вариант №%1.").arg(variant.number);
            return false;
        }
        numbers.insert(variant.number);
        loaded.append(variant);
    }
    std::sort(loaded.begin(), loaded.end(), [](const auto& left, const auto& right) {
        return left.number < right.number;
    });
    *variants = loaded;
    return true;
}
