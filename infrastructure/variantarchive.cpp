#include "infrastructure/variantarchive.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <openssl/evp.h>

#include <memory>
#include <algorithm>

namespace {
const QByteArray kMagic("M2MTOOLS2");
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
    parsed.source.kind = parsed.direction == ConversionDirection::MealyToMoore
        ? AutomatonKind::Mealy : AutomatonKind::Moore;
    if (!stringList(object.value(QStringLiteral("states")), &parsed.source.states) ||
        !stringList(object.value(QStringLiteral("inputs")), &parsed.source.inputs) ||
        !stringList(object.value(QStringLiteral("outputs")), &parsed.source.outputs))
        return fail(QStringLiteral("Архив содержит повреждённые списки имён."));
    parsed.source.initialState = parsed.source.states.indexOf(
        object.value(QStringLiteral("initialState")).toString());

    const QJsonValue transitionValue = object.value(QStringLiteral("transitions"));
    if (!transitionValue.isArray() ||
        transitionValue.toArray().size() != parsed.source.inputs.size())
        return fail(QStringLiteral("Архив содержит повреждённую таблицу переходов."));
    const QJsonArray rows = transitionValue.toArray();
    for (int input = 0; input < rows.size(); ++input) {
        if (!rows[input].isArray() || rows[input].toArray().size() != parsed.source.states.size())
            return fail(QStringLiteral("Размер таблицы переходов не совпадает со списками имён."));
        const QJsonArray cells = rows[input].toArray();
        for (int from = 0; from < cells.size(); ++from) {
            if (!cells[from].isObject()) return fail(QStringLiteral("Повреждена ячейка перехода."));
            const QJsonObject cell = cells[from].toObject();
            const QString nextName = cell.value(QStringLiteral("nextState")).toString();
            if (nextName.isEmpty()) continue;
            const int to = parsed.source.states.indexOf(nextName);
            if (to < 0) return fail(QStringLiteral("Переход ссылается на неизвестное состояние."));
            Transition transition{from, input, to, {}};
            QStringList outputNames;
            if (!stringList(cell.value(QStringLiteral("outputs")), &outputNames))
                return fail(QStringLiteral("Повреждены выходы перехода."));
            for (const QString& name : outputNames) {
                const int output = parsed.source.outputs.indexOf(name);
                if (output < 0) return fail(QStringLiteral("Переход ссылается на неизвестный выход."));
                transition.outputs.insert(output);
            }
            if (parsed.source.kind == AutomatonKind::Moore) transition.outputs.clear();
            parsed.source.transitions.append(transition);
        }
    }
    if (parsed.source.kind == AutomatonKind::Moore) {
        parsed.source.mooreOutputs.resize(parsed.source.states.size());
        const QJsonObject values = object.value(QStringLiteral("mooreOutputs")).toObject();
        for (int state = 0; state < parsed.source.states.size(); ++state) {
            QStringList names;
            if (!stringList(values.value(parsed.source.states[state]), &names))
                return fail(QStringLiteral("Повреждены выходы состояния автомата Мура."));
            for (const QString& name : names) {
                const int output = parsed.source.outputs.indexOf(name);
                if (output < 0) return fail(QStringLiteral("Состояние ссылается на неизвестный выход."));
                parsed.source.mooreOutputs[state].insert(output);
            }
        }
    }
    parsed.diagramFileName = object.value(QStringLiteral("imageName")).toString();
    parsed.diagram = QByteArray::fromBase64(
        object.value(QStringLiteral("imageData")).toString().toLatin1());
    QString validationError;
    if (!parsed.source.validate(&validationError))
        return fail(QStringLiteral("Вариант №%1 некорректен: %2")
                        .arg(parsed.number).arg(validationError));
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
        if (error) *error = QStringLiteral("Выбранный файл не является архивом M2MTools.");
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
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("M2MTools") ||
        root.value(QStringLiteral("version")).toInt() != 1 ||
        !root.value(QStringLiteral("variants")).isArray()) {
        if (error) *error = QStringLiteral("Версия BazaVar не поддерживается.");
        return false;
    }
    QList<TrainingVariant> loaded;
    QSet<int> numbers;
    for (const QJsonValue& value : root.value(QStringLiteral("variants")).toArray()) {
        if (!value.isObject()) return false;
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
