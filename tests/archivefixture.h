#pragma once
#include "domain/trainingvariant.h"
#include <QBuffer>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <memory>
#include <openssl/evp.h>

inline TrainingVariant trainingVariant() {
    TrainingVariant task;
    task.number = 7;
    task.direction = ConversionDirection::MooreToMealy;
    task.moore = {3, 2, 3};
    task.mealy = {2, 2, 3};
    task.reaction = {
        {"x2", {"y3", "y0"}}, {"x1", {"y2"}}, {"x2", {"y3", "y0"}}, {"x2", {}}, {"x1", {"y2"}}};
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::white);
    QBuffer buffer(&task.diagram);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    task.diagramFileName = "task.png";
    return task;
}

inline QJsonObject trainingVariantJson() {
    const auto task = trainingVariant();
    QJsonArray steps;
    for (const auto& step : task.reaction)
        steps.append(QJsonObject{{"input", step.input},
                                 {"outputs", QJsonArray::fromStringList(step.outputs)}});
    return {{"number", task.number},
            {"type", "MooreToMealy"},
            {"moore", QJsonObject{{"states", 3}, {"inputs", 2}, {"outputs", 3}}},
            {"mealy", QJsonObject{{"states", 2}, {"inputs", 2}, {"outputs", 3}}},
            {"reaction", steps},
            {"image", QJsonObject{{"fileName", task.diagramFileName},
                                  {"data", QString::fromLatin1(task.diagram.toBase64())}}}};
}

inline QJsonObject trainingArchiveJson() {
    return {{"format", "M2MToolsLite"},
            {"version", 1},
            {"variants", QJsonArray{trainingVariantJson()}}};
}

inline QByteArray archiveKeyMaterial() {
    const QByteArray left =
        QByteArray::fromHex("e758d0ffb756c99a9fed62dbb728a1e4cb06939e6d0d017dbb321b89a262c6e8");
    const QByteArray right =
        QByteArray::fromHex("91ed552426a3a7bc31a8c75f11d2ec96aec327628668c5aa074e99718add5531");
    QByteArray result(32, Qt::Uninitialized);
    for (int i = 0; i < result.size(); ++i)
        result[i] = left[i] ^ right[i];
    return result;
}

inline QByteArray makeCompatibleArchive(const QJsonObject& root = trainingArchiveJson()) {
    const QByteArray plain = QJsonDocument(root).toJson(QJsonDocument::Compact);
    const QByteArray magic("M2MLITE1");
    const QByteArray salt(16, 's');
    const QByteArray nonce(12, 'n');
    QByteArray key(32, Qt::Uninitialized);
    const QByteArray material = archiveKeyMaterial();
    require(PKCS5_PBKDF2_HMAC(material.constData(), material.size(),
                              reinterpret_cast<const unsigned char*>(salt.constData()), salt.size(),
                              200000, EVP_sha256(), key.size(),
                              reinterpret_cast<unsigned char*>(key.data())) == 1,
            "test archive key is derived");
    using Context = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;
    Context context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    require(bool(context), "test cipher context created");
    require(EVP_EncryptInit_ex(context.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
                EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) ==
                    1 &&
                EVP_EncryptInit_ex(context.get(), nullptr, nullptr,
                                   reinterpret_cast<const unsigned char*>(key.constData()),
                                   reinterpret_cast<const unsigned char*>(nonce.constData())) == 1,
            "test archive cipher initialized");
    int written = 0;
    require(EVP_EncryptUpdate(context.get(), nullptr, &written,
                              reinterpret_cast<const unsigned char*>(magic.constData()),
                              magic.size()) == 1,
            "test archive AAD added");
    QByteArray encrypted(plain.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int total = 0;
    require(EVP_EncryptUpdate(context.get(), reinterpret_cast<unsigned char*>(encrypted.data()),
                              &written, reinterpret_cast<const unsigned char*>(plain.constData()),
                              plain.size()) == 1,
            "test archive encrypted");
    total = written;
    require(EVP_EncryptFinal_ex(context.get(),
                                reinterpret_cast<unsigned char*>(encrypted.data()) + total,
                                &written) == 1,
            "test archive encryption finalized");
    encrypted.resize(total + written);
    QByteArray tag(16, Qt::Uninitialized);
    require(EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data()) == 1,
            "test archive tag created");
    return magic + salt + nonce + tag + encrypted;
}
