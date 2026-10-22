#include "domain/conversionservice.h"
#include "domain/isomorphismchecker.h"
#include "infrastructure/progressstore.h"
#include "infrastructure/variantarchive.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <openssl/evp.h>

#include <iostream>
#include <memory>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

Automaton sampleMoore() {
    Automaton result;
    result.kind = AutomatonKind::Moore;
    result.states = {QStringLiteral("a"), QStringLiteral("b")};
    result.inputs = {QStringLiteral("x")};
    result.outputs = {QStringLiteral("zero"), QStringLiteral("one")};
    result.initialState = 0;
    result.mooreOutputs = {{0}, {1}};
    result.transitions = {{0, 0, 1, {}}, {1, 0, 0, {}}};
    return result;
}

QByteArray archiveKeyMaterial() {
    const QByteArray left = QByteArray::fromHex(
        "e758d0ffb756c99a9fed62dbb728a1e4cb06939e6d0d017dbb321b89a262c6e8");
    const QByteArray right = QByteArray::fromHex(
        "91ed552426a3a7bc31a8c75f11d2ec96aec327628668c5aa074e99718add5531");
    QByteArray result(32, Qt::Uninitialized);
    for (int i = 0; i < result.size(); ++i) result[i] = left[i] ^ right[i];
    return result;
}

QByteArray makeCompatibleArchive() {
    QJsonObject first{{QStringLiteral("nextState"), QStringLiteral("b")},
                      {QStringLiteral("outputs"), QJsonArray{}}};
    QJsonObject second{{QStringLiteral("nextState"), QStringLiteral("a")},
                       {QStringLiteral("outputs"), QJsonArray{}}};
    QJsonObject mooreOutputs{{QStringLiteral("a"), QJsonArray{QStringLiteral("y")}},
                             {QStringLiteral("b"), QJsonArray{QStringLiteral("y")}}};
    QJsonObject variant{{QStringLiteral("number"), 7},
                        {QStringLiteral("type"), QStringLiteral("MooreToMealy")},
                        {QStringLiteral("states"), QJsonArray{QStringLiteral("a"), QStringLiteral("b")}},
                        {QStringLiteral("inputs"), QJsonArray{QStringLiteral("x")}},
                        {QStringLiteral("outputs"), QJsonArray{QStringLiteral("y")}},
                        {QStringLiteral("initialState"), QStringLiteral("a")},
                        {QStringLiteral("imageName"), QStringLiteral("task.png")},
                        {QStringLiteral("imageData"), QString::fromLatin1(QByteArray("image").toBase64())},
                        {QStringLiteral("transitions"), QJsonArray{QJsonArray{first, second}}},
                        {QStringLiteral("mooreOutputs"), mooreOutputs}};
    QJsonObject root{{QStringLiteral("format"), QStringLiteral("M2MTools")},
                     {QStringLiteral("version"), 1},
                     {QStringLiteral("variants"), QJsonArray{variant}}};
    const QByteArray plain = QJsonDocument(root).toJson(QJsonDocument::Compact);
    const QByteArray magic("M2MTOOLS2");
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
            EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, nonce.size(), nullptr) == 1 &&
            EVP_EncryptInit_ex(context.get(), nullptr, nullptr,
                reinterpret_cast<const unsigned char*>(key.constData()),
                reinterpret_cast<const unsigned char*>(nonce.constData())) == 1,
            "test archive cipher initialized");
    int written = 0;
    require(EVP_EncryptUpdate(context.get(), nullptr, &written,
            reinterpret_cast<const unsigned char*>(magic.constData()), magic.size()) == 1,
            "test archive AAD added");
    QByteArray encrypted(plain.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);
    int total = 0;
    require(EVP_EncryptUpdate(context.get(),
            reinterpret_cast<unsigned char*>(encrypted.data()), &written,
            reinterpret_cast<const unsigned char*>(plain.constData()), plain.size()) == 1,
            "test archive encrypted");
    total = written;
    require(EVP_EncryptFinal_ex(context.get(),
            reinterpret_cast<unsigned char*>(encrypted.data()) + total, &written) == 1,
            "test archive encryption finalized");
    encrypted.resize(total + written);
    QByteArray tag(16, Qt::Uninitialized);
    require(EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data()) == 1,
            "test archive tag created");
    return magic + salt + nonce + tag + encrypted;
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const Automaton expected = sampleMoore();
    Automaton renamed;
    renamed.kind = AutomatonKind::Moore;
    renamed.states = {QStringLiteral("right"), QStringLiteral("left")};
    renamed.inputs = {QStringLiteral("button")};
    renamed.outputs = {QStringLiteral("bright"), QStringLiteral("dark")};
    renamed.initialState = 1;
    renamed.mooreOutputs = {{0}, {1}};
    renamed.transitions = {{1, 0, 0, {}}, {0, 0, 1, {}}};
    require(IsomorphismChecker::compare(expected, renamed).equivalent,
            "renamed states, inputs and outputs are accepted");
    renamed.transitions[0].to = 1;
    require(!IsomorphismChecker::compare(expected, renamed).equivalent,
            "changed transition is rejected");

    Automaton mealy;
    QString error;
    require(ConversionService::convert(expected, &mealy, &error),
            "Moore-to-Mealy conversion succeeds");
    require(mealy.kind == AutomatonKind::Mealy && mealy.states.size() == 2,
            "Moore-to-Mealy preserves states");
    require(mealy.transitions[0].outputs == QSet<int>{1} &&
            mealy.transitions[1].outputs == QSet<int>{0},
            "Moore-to-Mealy places destination outputs on transitions");

    Automaton convertedBack;
    require(ConversionService::convert(mealy, &convertedBack, &error),
            "Mealy-to-Moore conversion succeeds");
    require(convertedBack.kind == AutomatonKind::Moore &&
            convertedBack.states.size() == 3,
            "Mealy-to-Moore creates output-split states and synthetic initial state");

    QTemporaryDir temporary;
    require(temporary.isValid(), "temporary directory created");
    const QString progress = temporary.filePath(QStringLiteral("work.m2mwork"));
    QJsonObject saved{{QStringLiteral("variantNumber"), 17},
                      {QStringLiteral("student"), QStringLiteral("Test")}};
    require(ProgressStore::save(progress, saved, &error), "progress saves atomically");
    QJsonObject loaded;
    require(ProgressStore::load(progress, &loaded, &error) && loaded == saved,
            "progress round trip preserves data");

    const QString archivePath = temporary.filePath(QStringLiteral("BazaVar.m2m"));
    QFile archiveFile(archivePath);
    require(archiveFile.open(QIODevice::WriteOnly), "compatible archive fixture opened");
    const QByteArray archive = makeCompatibleArchive();
    require(archiveFile.write(archive) == archive.size(), "compatible archive fixture written");
    archiveFile.close();
    QList<TrainingVariant> variants;
    require(VariantArchive::load(archivePath, &variants, &error) &&
            variants.size() == 1 && variants.first().number == 7 &&
            variants.first().source.kind == AutomatonKind::Moore,
            "M2MTools-compatible encrypted BazaVar is loaded");
    QByteArray modified = archive;
    modified[modified.size() - 1] ^= 1;
    require(archiveFile.open(QIODevice::WriteOnly | QIODevice::Truncate),
            "tampered archive fixture opened");
    require(archiveFile.write(modified) == modified.size(), "tampered archive fixture written");
    archiveFile.close();
    require(!VariantArchive::load(archivePath, &variants, &error),
            "authenticated BazaVar modification is rejected");
    std::cout << "PASS conversion, weighted graph isomorphism, BazaVar and progress round trip\n";
    return 0;
}
