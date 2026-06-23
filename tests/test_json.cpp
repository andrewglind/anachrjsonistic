#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "json.h"

// ---------------------------------------------------------------------------
// Helpers from the README's "Optional values with fallbacks" section
// ---------------------------------------------------------------------------

static bool OptionalBool(json::jobject& obj, const std::string& key, bool fallback) {
    return obj.hasKey(key) ? (bool) obj[key] : fallback;
}

static int OptionalInt(json::jobject& obj, const std::string& key, int fallback) {
    return obj.hasKey(key) ? (int) obj[key] : fallback;
}

static std::string OptionalString(json::jobject& obj, const std::string& key, const std::string& fallback) {
    return obj.hasKey(key) ? (std::string) obj[key] : fallback;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("reading scalar values") {
    json::jobject config = json::parse(
        "{ \"width\": 800, \"enabled\": true, \"title\": \"example\" }");

    int width = config["width"];
    bool enabled = config["enabled"];
    std::string title = config["title"];

    CHECK(width == 800);
    CHECK(enabled);
    CHECK(title == "example");
}

TEST_CASE("toFloat parses the stored text on read") {
    json::jobject config = json::parse("{ \"scale\": 1.5, \"ratio\": 0.25 }");

    float scale = config["scale"];
    float ratio = config["ratio"];

    CHECK(scale == doctest::Approx(1.5f));
    CHECK(ratio == doctest::Approx(0.25f));
}

TEST_CASE("absent key yields a safe zero value rather than throwing") {
    json::jobject config = json::parse("{ \"present\": \"yes\" }");

    CHECK(!config.hasKey("absent"));

    int i = config["absent"];
    float f = config["absent"];
    bool b = config["absent"];
    std::string s = config["absent"];

    CHECK(i == 0);
    CHECK(f == doctest::Approx(0.0f));
    CHECK(!b);
    CHECK(s == "");
}

TEST_CASE("reading a scalar with the wrong-typed accessor is also safe") {
    json::jobject config = json::parse("{ \"title\": \"example\", \"count\": 7 }");

    // A string read as a number falls back to zero (strtol of "example").
    int asInt = config["title"];
    CHECK(asInt == 0);

    // A number read as a string is still the original text.
    std::string asStr = config["count"];
    CHECK(asStr == "7");
}

TEST_CASE("toBool recognises the documented truthy spellings and nothing else") {
    json::jobject config = json::parse(
        "{ \"a\": true, \"b\": 1, \"c\": yes, \"d\": false, \"e\": 0, \"f\": maybe }");

    CHECK((bool) config["a"]);
    CHECK((bool) config["b"]);
    CHECK((bool) config["c"]);
    CHECK(!(bool) config["d"]);
    CHECK(!(bool) config["e"]);
    CHECK(!(bool) config["f"]);
}

TEST_CASE("optional values with fallbacks") {
    json::jobject config = json::parse(
        "{ \"name\": \"set\", \"timeout\": 30, \"verbose\": true }");

    CHECK(OptionalString(config, "name", "fallback") == "set");
    CHECK(OptionalString(config, "missing", "fallback") == "fallback");

    CHECK(OptionalInt(config, "timeout", 10) == 30);
    CHECK(OptionalInt(config, "missing", 10) == 10);

    CHECK(OptionalBool(config, "verbose", false));
    CHECK(OptionalBool(config, "missing", true));
}

TEST_CASE("nested objects: assign-to-jobject and chained access") {
    json::jobject config = json::parse(
        "{ \"display\": { \"refresh_rate\": 120 }, \"buffer\": { \"size\": 4096 } }");

    json::jobject display = config["display"];
    int rate = OptionalInt(display, "refresh_rate", 60);
    CHECK(rate == 120);

    // Fallback when the nested key is absent.
    int depth = OptionalInt(display, "depth", 24);
    CHECK(depth == 24);

    // ...or chain straight through.
    int size = config["buffer"]["size"];
    CHECK(size == 4096);
}

TEST_CASE("arrays of objects") {
    json::jobject config = json::parse(
        "{ \"items\": ["
        "  { \"name\": \"alpha\", \"active\": true },"
        "  { \"name\": \"beta\", \"active\": false },"
        "  { \"name\": \"gamma\" }"
        "] }");

    CHECK(config.hasKey("items"));

    std::vector<json::jobject> items = config["items"];
    REQUIRE(items.size() == 3);

    // items[N] is a jobject (vector element).
    CHECK((std::string) items[0]["name"] == "alpha");
    CHECK(OptionalBool(items[0], "active", true));

    CHECK((std::string) items[1]["name"] == "beta");
    CHECK(!OptionalBool(items[1], "active", true));

    // Missing "active" falls back to the default.
    CHECK((std::string) items[2]["name"] == "gamma");
    CHECK(OptionalBool(items[2], "active", true));
}

TEST_CASE("array element access via the proxy index operator") {
    json::jobject config = json::parse(
        "{ \"items\": [ { \"id\": 10 }, { \"id\": 20 } ] }");

    json::proxy items = config["items"];
    CHECK(items.isArray());

    CHECK((int) items[0]["id"] == 10);
    CHECK((int) items[1]["id"] == 20);

    // Out-of-range and negative indices yield an invalid proxy / safe defaults.
    json::proxy oob = items[5];
    CHECK(!oob.isValid());
    CHECK((int) items[5]["id"] == 0);
    CHECK((int) items[-1]["id"] == 0);
}

TEST_CASE("chained access: string-literal keys and integer indexing") {
    json::jobject config = json::parse(
        "{ \"a\": { \"b\": { \"c\": \"deep\" } },"
        "  \"list\": [ { \"v\": \"first\" }, { \"v\": \"second\" } ] }");

    // Nested string-literal keys.
    CHECK((std::string) config["a"]["b"]["c"] == "deep");

    // Integer indexing into the array.
    json::proxy list = config["list"];
    CHECK((std::string) list[0]["v"] == "first");
    CHECK((std::string) list[1]["v"] == "second");

    // Mixed key, index, key.
    CHECK((std::string) config["list"][0]["v"] == "first");
}

TEST_CASE("reading an object as a string map") {
    json::jobject config = json::parse(
        "{ \"entries\": { \"a\": \"one\", \"b\": \"two\", \"c\": \"three\" } }");

    json::jobject entries = config["entries"];
    std::map<std::string, std::string> files = entries.toMap();

    REQUIRE(files.size() == 3);
    CHECK(files["a"] == "one");
    CHECK(files["b"] == "two");
    CHECK(files["c"] == "three");
}

TEST_CASE("toMap collects only scalar-string values, skipping objects/arrays") {
    json::jobject config = json::parse(
        "{ \"mixed\": { \"name\": \"x\", \"nested\": { \"k\": \"v\" }, \"list\": [ \"1\" ] } }");

    json::jobject mixed = config["mixed"];
    std::map<std::string, std::string> m = mixed.toMap();

    REQUIRE(m.size() == 1);
    CHECK(m["name"] == "x");
    CHECK(m.find("nested") == m.end());
    CHECK(m.find("list") == m.end());
}

TEST_CASE("building and inspecting values") {
    json::jobject obj;  // a fresh, empty object
    CHECK(!obj.hasKey("name"));

    obj.setString("name", "example");
    CHECK(obj.hasKey("name"));
    CHECK((std::string) obj["name"] == "example");

    // setString on an existing key overwrites in place.
    obj.setString("name", "updated");
    CHECK((std::string) obj["name"] == "updated");
}

TEST_CASE("setObject nests a built object readable through chained access") {
    json::jobject child;
    child.setString("inner", "value");

    json::jobject parent;
    parent.setObject("child", child);

    CHECK(parent.hasKey("child"));
    json::proxy node = parent["child"];
    CHECK(node.isObject());
    CHECK((std::string) parent["child"]["inner"] == "value");
}

TEST_CASE("proxy type predicates and the isObject guard") {
    json::jobject config = json::parse(
        "{ \"section\": { \"k\": \"v\" }, \"label\": \"text\", \"list\": [ \"a\" ] }");

    json::proxy section = config["section"];
    CHECK(section.isValid());
    CHECK(section.isObject());
    CHECK(!section.isString());
    CHECK(!section.isArray());

    json::proxy label = config["label"];
    CHECK(label.isString());
    CHECK(!label.isObject());

    json::proxy list = config["list"];
    CHECK(list.isArray());
    CHECK(!list.isObject());

    json::proxy missing = config["nope"];
    CHECK(!missing.isValid());
    CHECK(!missing.isObject());
    CHECK(!missing.isString());
    CHECK(!missing.isArray());

    // The guarded read pattern from the README.
    json::proxy node = config["section"];
    REQUIRE(node.isObject());
    json::jobject s = node;
    CHECK((std::string) s["k"] == "v");
}

TEST_CASE("string escape handling in the parser") {
    json::jobject config = json::parse(
        "{ \"path\": \"a\\tb\", \"line\": \"x\\ny\", \"quote\": \"he said \\\"hi\\\"\" }");

    CHECK((std::string) config["path"] == "a\tb");
    CHECK((std::string) config["line"] == "x\ny");
    CHECK((std::string) config["quote"] == "he said \"hi\"");
}

TEST_CASE("empty object and assorted whitespace are tolerated") {
    json::jobject empty = json::parse("{}");
    CHECK(!empty.hasKey("anything"));

    json::jobject spaced = json::parse("  {\n\t \"k\" : \"v\" \r\n}  ");
    CHECK((std::string) spaced["k"] == "v");
}
