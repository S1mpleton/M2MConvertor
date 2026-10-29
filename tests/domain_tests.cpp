#include "domain/conversionservice.h"
#include "domain/reactionchecker.h"
#include "domain/equivalencechecker.h"
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
#include <algorithm>
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


}

#include "tests/archivefixture.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    auto task = trainingVariant();
    task.reaction = {{"x2", {"y2"}}, {"x1", {"y3", "y0"}},
                     {"x2", {"y2"}}, {"x2", {"y3", "y0"}}};
    Automaton reactionMoore;
    reactionMoore.kind = AutomatonKind::Moore;
    reactionMoore.states = {"a0", "a1", "a2"};
    reactionMoore.initialState = 0;
    reactionMoore.inputs = {"x1", "x2"};
    reactionMoore.outputs = {"y0", "y2", "y3"};
    reactionMoore.mooreOutputs = {{1}, {0, 2}, {}};
    reactionMoore.transitions = {{0, 1, 1, {}}, {1, 0, 0, {}},
                                 {1, 1, 2, {}}, {2, 0, 0, {}}};
    require(ReactionChecker::check(task, reactionMoore).accepted,
            "Moore consumes ordered reaction from a0 using current-state outputs");
    auto broken = reactionMoore;
    broken.mooreOutputs[0] = {0};
    require(ReactionChecker::check(task, broken).step == 0,
            "first incorrect output is identified");
    broken = reactionMoore;
    broken.transitions.removeFirst();
    require(ReactionChecker::check(task, broken).step == 0,
            "missing transition reports exact missing step");
    broken = reactionMoore;
    broken.initialState = 1;
    require(!ReactionChecker::check(task, broken).accepted, "initial state must be a0");
    broken = reactionMoore;
    broken.states[0] = "q0";
    require(!ReactionChecker::check(task, broken).accepted, "state names are canonical");
    broken = reactionMoore;
    broken.inputs[1] = "x9";
    require(!ReactionChecker::check(task, broken).accepted, "signals are not silently renamed");
    broken = reactionMoore;
    broken.outputs[0] = "y10";
    require(!ReactionChecker::check(task, broken).accepted, "two-digit signals are invalid");
    auto reordered = task;
    std::swap(reordered.reaction[0], reordered.reaction[1]);
    require(!ReactionChecker::check(reordered, reactionMoore).accepted,
            "reaction order affects the verdict");
    reordered = task;
    reordered.reaction[1].outputs = {"y0", "y3"};
    require(ReactionChecker::check(reordered, reactionMoore).accepted,
            "outputs within one clock tick are a set");
    Automaton reactionMealy;
    reactionMealy.kind = AutomatonKind::Mealy;
    reactionMealy.states = {"a0", "a1"};
    reactionMealy.initialState = 0;
    reactionMealy.inputs = reactionMoore.inputs;
    reactionMealy.outputs = reactionMoore.outputs;
    reactionMealy.transitions = {{0, 1, 1, {1}}, {1, 0, 0, {0, 2}}, {1, 1, 1, {0, 2}}};
    require(ReactionChecker::check(task, reactionMealy).accepted,
            "Mealy uses its own counts and is checked against the same trace");
    auto wrongCounts = task;
    wrongCounts.mealy.states = 3;
    require(!ReactionChecker::check(wrongCounts, reactionMealy).accepted &&
             ReactionChecker::check(wrongCounts, reactionMoore).accepted,
            "counts are selected separately for each automaton kind");
    wrongCounts = task;
    wrongCounts.mealy.outputs = 2;
    require(!ReactionChecker::check(wrongCounts, reactionMealy).accepted,
            "actual distinct output count must match the task");
    wrongCounts.direction = ConversionDirection::MealyToMoore;
    require(!ReactionChecker::check(wrongCounts, reactionMoore).accepted,
            "unfinished conversion is unavailable in the checker");
    TrainingVariant single;
    single.moore = single.mealy = {1, 1, 1};
    single.reaction = {{"x9", {"y4"}}};
    Automaton one;
    one.kind = AutomatonKind::Moore; one.states = {"a0"}; one.initialState = 0;
    one.inputs = {"x9"}; one.outputs = {"y4"}; one.mooreOutputs = {{0}};
    require(ReactionChecker::check(single, one).accepted, "single Moore output needs no final transition");
    one.transitions = {{0, 0, 0, {}}};
    require(ReactionChecker::check(single, one).accepted, "single Moore output also accepts a loop");
    single.reaction.append(ReactionStep{"x9", {"y4"}});
    require(ReactionChecker::check(single, one).accepted, "loop carries the reaction to the next tick");
    one.transitions.clear();
    require(!ReactionChecker::check(single, one).accepted, "missing nonfinal transition cannot continue");
    single.reaction.removeLast();
    one.states.append("a1"); one.mooreOutputs.append(QSet<int>{});
    require(ReactionChecker::check(single, one, true).accepted &&
            !ReactionChecker::check(single, one).accepted, "graph accepts extra states but table uses variant counts");
    one.states = ReactionChecker::stateNames(11); one.mooreOutputs.resize(11);
    require(!ReactionChecker::check(single, one, true).accepted, "graph cannot contain more than 10 states");
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
    require(EquivalenceChecker::compare(mealy, convertedBack).equivalent,
            "constructed Moore automaton is behaviorally equivalent");

    // Regression for a one-state Mealy loop q1 --x1/{y1,y2}--> q1. A Moore
    // loop with {y1,y2} on its only state has the same reaction after every
    // consumed input, even though it is not isomorphic to the two-state
    // construction with a synthetic initial state.
    Automaton oneStateMealy;
    oneStateMealy.kind = AutomatonKind::Mealy;
    oneStateMealy.states = {QStringLiteral("q1")};
    oneStateMealy.inputs = {QStringLiteral("x1")};
    oneStateMealy.outputs = {QStringLiteral("y1"), QStringLiteral("y2")};
    oneStateMealy.initialState = 0;
    oneStateMealy.transitions = {{0, 0, 0, {0, 1}}};
    Automaton oneStateMoore;
    oneStateMoore.kind = AutomatonKind::Moore;
    oneStateMoore.states = {QStringLiteral("renamed")};
    oneStateMoore.inputs = {QStringLiteral("input")};
    oneStateMoore.outputs = {QStringLiteral("outA"), QStringLiteral("outB")};
    oneStateMoore.initialState = 0;
    oneStateMoore.mooreOutputs = {{0, 1}};
    oneStateMoore.transitions = {{0, 0, 0, {}}};
    require(EquivalenceChecker::compare(oneStateMealy, oneStateMoore).equivalent,
            "one-state Mealy and Moore loops with the same reaction are equivalent");

    Automaton redundantMoore = oneStateMoore;
    redundantMoore.states.append(QStringLiteral("redundant"));
    redundantMoore.mooreOutputs.append({0, 1});
    redundantMoore.transitions = {{0, 0, 1, {}}, {1, 0, 1, {}}};
    require(EquivalenceChecker::compare(oneStateMealy, redundantMoore).equivalent,
            "reachable behaviorally indistinguishable states are minimized");
    oneStateMoore.mooreOutputs[0] = {0};
    require(!EquivalenceChecker::compare(oneStateMealy, oneStateMoore).equivalent,
            "different output reaction is rejected");

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
            variants.first().direction == ConversionDirection::MooreToMealy &&
            variants.first().moore.states == 3 && variants.first().mealy.states == 2 &&
            variants.first().diagram == trainingVariant().diagram &&
            variants.first().reaction[0].input == "x2" &&
            variants.first().reaction[0].outputs == QStringList({"y3", "y0"}) &&
            variants.first().reaction[1].input == "x1" &&
            variants.first().reaction[3].outputs.isEmpty(),
            "M2MTools-compatible encrypted BazaVar is loaded");
    QByteArray modified = archive;
    modified[modified.size() - 1] ^= 1;
    require(archiveFile.open(QIODevice::WriteOnly | QIODevice::Truncate),
            "tampered archive fixture opened");
    require(archiveFile.write(modified) == modified.size(), "tampered archive fixture written");
    archiveFile.close();
    require(!VariantArchive::load(archivePath, &variants, &error),
            "authenticated BazaVar modification is rejected");
    // Authenticated payload validation is separate from decryption.
    auto loadRoot = [&](const QJsonObject& root) {
        require(archiveFile.open(QIODevice::WriteOnly | QIODevice::Truncate), "fixture reopens");
        archiveFile.write(makeCompatibleArchive(root));
        archiveFile.close();
        return VariantArchive::load(archivePath, &variants, &error);
    };
    auto root = trainingArchiveJson();
    auto malformed = trainingVariantJson();
    malformed.insert("moore", QJsonObject{{"states", 3}, {"inputs", 1}, {"outputs", 3}});
    root.insert("variants", QJsonArray{malformed});
    require(!loadRoot(root), "source counts inconsistent with trace are rejected");
    malformed = trainingVariantJson();
    auto steps = malformed.value("reaction").toArray();
    steps[0] = QJsonObject{{"input", "x2"}, {"outputs", QJsonArray{"y3", "y3"}}};
    malformed.insert("reaction", steps);
    root.insert("variants", QJsonArray{malformed});
    require(!loadRoot(root), "duplicate outputs in an archived tick are rejected");
    malformed = trainingVariantJson();
    malformed.insert("image", QJsonObject{{"fileName", "broken.png"}, {"data", "!!!!"}});
    root.insert("variants", QJsonArray{malformed});
    require(!loadRoot(root), "corrupted image is rejected");
    root.insert("variants", QJsonArray{trainingVariantJson(), trainingVariantJson()});
    require(!loadRoot(root), "duplicate task numbers are rejected");
    root = trainingArchiveJson();
    root.insert("format", "M2MTools");
    require(!loadRoot(root), "old full-automaton payload is rejected");
    require(variants.size() == 1 && variants.first().reaction.size() == 5,
            "failed imports do not overwrite the loaded task list");
    if (argc > 1) {
        require(VariantArchive::load(QString::fromLocal8Bit(argv[1]), &variants, &error) &&
                variants.size() == 1 && variants.first().number == 7 &&
                variants.first().moore.states == 3 && variants.first().mealy.states == 2 &&
                variants.first().reaction.size() == 5 &&
                variants.first().reaction[0].outputs == QStringList({"y3", "y0"}) &&
                !variants.first().diagram.isEmpty(),
                "real M2MTools export preserves image, counts and ordered reaction");
    }
    std::cout << "PASS conversion, behavioral equivalence, graph isomorphism, BazaVar and progress round trip\n";
    return 0;
}
