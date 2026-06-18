package json;

// ---------------------------------------------------------------------------
// JType — the three value shapes (the original `jvalue::Type`). An `Int`-backed
// `enum abstract` lowers to a plain C++ `enum`.
// ---------------------------------------------------------------------------
enum abstract JType(Int) {
	var Str;	// 0 — a scalar string (numbers/bools are stored as their text too)
	var Obj;	// 1 — an object: parallel key/value arrays
	var Arr;	// 2 — an array of values
}

// ---------------------------------------------------------------------------
// JValue — the value node (C++ `json::jvalue`). A reference type (Hatchet emits
// it as a heap class, referenced by pointer), so `findKey` hands back a live
// node and `Proxy` can wrap one — just as the original returned a `jvalue*`. An
// object is parallel key/value arrays (insertion order preserved); an array is a
// list of child values.
// ---------------------------------------------------------------------------
@:native("jvalue")
class JValue {
	public var type:JType;
	public var s:String;
	public var objectKeys:Array<String>;
	public var objectVals:Array<JValue>;
	public var array:Array<JValue>;

	public function new() {
		this.type = JType.Str;
		this.s = "";
		this.objectKeys = [];
		this.objectVals = [];
		this.array = [];
	}

	// A scalar value holding `v` (the parser stores numbers and booleans as text,
	// just as the C++ version did — conversion happens at read time in `Proxy`).
	public static function ofString(v:String):JValue {
		var j = new JValue();
		j.type = JType.Str;
		j.s = v;
		return j;
	}

	public static function ofArray(a:Array<JValue>):JValue {
		var j = new JValue();
		j.type = JType.Arr;
		j.array = a;
		return j;
	}

	// The value for `key`, or `null` if this node is not an object / has no such
	// key (the `jvalue*` / null of the original).
	public function findKey(key:String):JValue {
		for (i in 0...this.objectKeys.length) {
			if (this.objectKeys[i] == key) {
				return this.objectVals[i];
			}
		}
		return null;
	}

	// Insert or replace `key`, preserving first-seen order.
	public function setKey(key:String, val:JValue):Void {
		for (i in 0...this.objectKeys.length) {
			if (this.objectKeys[i] == key) {
				this.objectVals[i] = val;
				return;
			}
		}
		this.objectKeys.push(key);
		this.objectVals.push(val);
	}

	public function isString():Bool {
		return this.type == JType.Str;
	}

	public function isObject():Bool {
		return this.type == JType.Obj;
	}

	public function isArray():Bool {
		return this.type == JType.Arr;
	}
}

// ---------------------------------------------------------------------------
// JObject — a JSON object (C++ `json::jobject`). An `abstract` over a `JValue`:
// a zero-overhead typed view of an object-kind node, so `obj["key"]` is a real
// C++ `operator[]` (via `@:op([])`) returning a `Proxy`. `this` inside these
// methods is the underlying `JValue`.
// ---------------------------------------------------------------------------
@:native("jobject")
abstract JObject(JValue) {
	// `new JObject()` makes a fresh, empty object node; `new JObject(existing)`
	// adopts an existing object-kind `JValue` (reading a nested object back out —
	// the equivalent of the C++ `out.root = *val`).
	public function new(?root:JValue) {
		if (root == null) {
			this = new JValue();
			this.type = JType.Obj;
		} else {
			this = root;
		}
	}

	// The underlying node — the analogue of the public `jobject::root` member,
	// used by the parser when nesting an object inside an array.
	public function root():JValue {
		return this;
	}

	public function hasKey(key:String):Bool {
		return this.findKey(key) != null;
	}

	public function setString(key:String, value:String):Void {
		this.setKey(key, JValue.ofString(value));
	}

	public function setObject(key:String, child:JObject):Void {
		this.setKey(key, child.root());
	}

	public function setArray(key:String, arr:Array<JValue>):Void {
		this.setKey(key, JValue.ofArray(arr));
	}

	public function toMap():Map<String, String> {
		var out = new Map<String, String>();
		for (i in 0...this.objectKeys.length) {
			if (this.objectVals[i].isString()) {
				out.set(this.objectKeys[i], this.objectVals[i].s);
			}
		}
		return out;
	}

	// `obj["key"]` → a `Proxy` over the value (or a null `Proxy` if absent).
	@:op([]) public function get(key:String):Proxy {
		return new Proxy(this.findKey(key));
	}
}

// ---------------------------------------------------------------------------
// Proxy — a read cursor into a JSON value (C++ `json::proxy`), and the whole
// reason `abstract` types exist in Hatchet. An `abstract` wrapping a (possibly
// null) `JValue`:
//
//   * `p["key"]` / `p[i]` are real `operator[]` overloads (`@:op([])`),
//   * an implicit cast to `String`/`Int`/`cpp.Float32`/`Bool`/`JObject`/`Array<JObject>`
//     is a real C++ conversion operator (`@:to`),
//
// so a consumer writes `var n:Int = obj["count"];` and the C++ compiler picks
// the conversion. A null wrap is the "missing / wrong type" sentinel (the
// original's null `jvalue*`).
// ---------------------------------------------------------------------------
@:native("proxy")
abstract Proxy(JValue) {
	public function new(v:JValue) {
		this = v;
	}

	// `p["key"]` — descend into an object (null `Proxy` unless this is an object).
	@:op([]) public function key(k:String):Proxy {
		if (this == null || !this.isObject()) {
			return new Proxy(null);
		}
		return new Proxy(this.findKey(k));
	}

	// `p[i]` — index into an array (null `Proxy` unless this is an array and `i`
	// is in range).
	@:op([]) public function index(i:Int):Proxy {
		if (this == null || !this.isArray()) {
			return new Proxy(null);
		}
		if (i < 0 || i >= this.array.length) {
			return new Proxy(null);
		}
		return new Proxy(this.array[i]);
	}

	@:to public function toStr():String {
		if (this == null || !this.isString()) {
			return "";
		}
		return this.s;
	}

	@:to public function toInt():Int {
		if (this == null || !this.isString()) {
			return 0;
		}
		return Std.parseInt(this.s);
	}

	@:to public function toFloat():cpp.Float32 {
		if (this == null || !this.isString()) {
			return cast(0.0, cpp.Float32);
		}
		return cast(Std.parseFloat(this.s), cpp.Float32);
	}

	@:to public function toBool():Bool {
		if (this == null || !this.isString()) {
			return false;
		}
		return this.s == "true" || this.s == "1" || this.s == "yes";
	}

	// Read the value back as an object (empty object when it is not one).
	@:to public function toObject():JObject {
		if (this != null && this.isObject()) {
			return new JObject(this);
		}
		return new JObject();
	}

	// Read an array as a list of objects, wrapping scalar/array elements the same
	// way the original `operator std::vector<jobject>()` did.
	@:to public function toObjectArray():Array<JObject> {
		var out:Array<JObject> = [];
		if (this == null || !this.isArray()) {
			return out;
		}
		for (i in 0...this.array.length) {
			var elem = this.array[i];
			if (elem.isObject()) {
				out.push(new JObject(elem));
			} else if (elem.isString()) {
				var o = new JObject();
				o.setString("value", elem.s);
				out.push(o);
			} else if (elem.isArray()) {
				var o = new JObject();
				o.setArray("array", elem.array);
				out.push(o);
			}
		}
		return out;
	}

	public function isValid():Bool {
		return this != null;
	}

	public function isString():Bool {
		return this != null && this.isString();
	}

	public function isObject():Bool {
		return this != null && this.isObject();
	}

	public function isArray():Bool {
		return this != null && this.isArray();
	}
}

// ---------------------------------------------------------------------------
// Json — the parser + entry point (the free `json::parse`, `parse_object`,
// `parse_array`, … functions). The C++ version threaded a `const char*& p`
// cursor; here the cursor is a `String` plus an `Int` position, which Hatchet
// lowers to a `std::string` and an index. Deliberately lenient (the original's
// behaviour): unquoted scalars are kept as text, malformed input is skipped.
// ---------------------------------------------------------------------------
class Json {
	var text:String;
	var pos:Int;

	public function new(text:String) {
		this.text = text;
		this.pos = 0;
	}

	// The character at the cursor, or "" at end of input (`std::string::substr`
	// returns empty at `size()`), which every loop below treats as the terminator.
	function cur():String {
		return this.text.charAt(this.pos);
	}

	public function skipWs():Void {
		var c = cur();
		while (c == " " || c == "\n" || c == "\t" || c == "\r") {
			this.pos++;
			c = cur();
		}
	}

	function parseString():String {
		var out = "";
		this.pos++; // skip opening quote
		var c = cur();
		while (c != "" && c != "\"") {
			if (c == "\\") {
				this.pos++;
				var e = cur();
				if (e == "") {
					break; // trailing backslash at end of input
				}
				if (e == "n") {
					out += "\n";
				} else if (e == "t") {
					out += "\t";
				} else if (e == "r") {
					out += "\r";
				} else {
					out += e;
				}
				this.pos++;
			} else {
				out += c;
				this.pos++;
			}
			c = cur();
		}
		if (cur() == "\"") {
			this.pos++; // skip closing quote
		}
		return out;
	}

	function parseUnquoted():String {
		var start = this.pos;
		var c = cur();
		while (c != "" && c != "," && c != "}" && c != "]" && c != " " && c != "\n" && c != "\t" && c != "\r") {
			this.pos++;
			c = cur();
		}
		return this.text.substring(start, this.pos);
	}

	public function parseObject():JObject {
		var obj = new JObject();
		this.pos++; // skip '{'
		skipWs();
		var c = cur();
		while (c != "" && c != "}") {
			if (c != "\"") {
				this.pos++; // skip malformed input
				c = cur();
				continue;
			}
			var key = parseString();
			skipWs();
			if (cur() == ":") {
				this.pos++;
			}
			skipWs();
			var d = cur();
			if (d == "{") {
				obj.setObject(key, parseObject());
			} else if (d == "[") {
				obj.setArray(key, parseArray());
			} else if (d == "\"") {
				obj.setString(key, parseString());
			} else {
				obj.setString(key, parseUnquoted());
			}
			skipWs();
			if (cur() == ",") {
				this.pos++;
				skipWs();
			}
			c = cur();
		}
		if (cur() == "}") {
			this.pos++;
		}
		return obj;
	}

	function parseArray():Array<JValue> {
		var arr:Array<JValue> = [];
		this.pos++; // skip '['
		skipWs();
		var c = cur();
		while (c != "" && c != "]") {
			if (c == "{") {
				var child = parseObject();
				arr.push(child.root());
			} else if (c == "[") {
				arr.push(JValue.ofArray(parseArray()));
			} else if (c == "\"") {
				arr.push(JValue.ofString(parseString()));
			} else {
				arr.push(JValue.ofString(parseUnquoted()));
			}
			skipWs();
			if (cur() == ",") {
				this.pos++;
				skipWs();
			}
			c = cur();
		}
		if (cur() == "]") {
			this.pos++;
		}
		return arr;
	}
}

// Parse a JSON document, returning its root object.
function parse(text:String):JObject {
	var p = new Json(text);
	p.skipWs();
	return p.parseObject();
}
