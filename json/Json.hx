package json;

// ---------------------------------------------------------------------------
// JType — the three value shapes
// ---------------------------------------------------------------------------
enum abstract JType(Int) {
	var Str = 0;	// a scalar string (numbers/bools are stored as their text too)
	var Obj = 1;	// an ordered map of key/value pairs
	var Arr = 2;	// an array of values
}

// ---------------------------------------------------------------------------
// JValue — the value node (C++ `json::jvalue`)
// ---------------------------------------------------------------------------
@:native("jvalue")
class JValue {
	public var type:JType;
	public var s:String;
	@orderedMap public var objects:Map<String, JValue>;
	public var array:Array<JValue>;

	public function new() {
		this.type = JType.Str;
		this.s = "";
		this.objects = [];
		this.array = [];
	}

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

	// The value for `key`, or `null` if this node is not an object / has no such key
	public function findKey(key:String):JValue {
		return this.objects.get(key);
	}

	// Insert or replace `key`, preserving first-seen order
	public function setKey(key:String, val:JValue):Void {
		this.objects.set(key, val);
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
// JObject — a JSON object (C++ `json::jobject`)
// ---------------------------------------------------------------------------
@:native("jobject")
abstract JObject(JValue) {
	public function new(?root:JValue) {
		if (root == null) {
			this = new JValue();
			this.type = JType.Obj;
		} else {
			this = root;
		}
	}

	// The underlying node
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
		return [ for (k => v in this.objects) if(v.isString()) k => v.s ];
	}

	// A `Proxy` over the value
	@:op([]) public function get(key:String):Proxy {
		return new Proxy(this.findKey(key));
	}
}

// ---------------------------------------------------------------------------
// Proxy — a read cursor into a JSON value (C++ `json::proxy`)
// ---------------------------------------------------------------------------
@:native("proxy")
abstract Proxy(JValue) {
	public function new(v:JValue) {
		this = v;
	}

	// Descend into an object (null `Proxy` unless this is an object)
	@:op([]) public function key(k:String):Proxy {
		return new Proxy((this?.isObject() ?? false) ? this.findKey(k) : null);
	}

	// A `const char*` overload of `[]` so chaining a plain string literal stays unambiguous
	@:op([]) public function keyChar(k:cpp.ConstCharStar):Proxy {
		return key((k : String));
	}

	// Index into an array (null `Proxy` unless this is an array and `i` is in range)
	@:op([]) public function index(i:Int):Proxy {
		if (!(this?.isArray() ?? false) || i < 0 || i >= this.array.length) {
			return new Proxy(null);
		}
		return new Proxy(this.array[i]);
	}

	@:to public function toStr():String {
		return (this?.isString() ?? false) ? this.s : "";
	}

	@:to public function toInt():Int {
		return (this?.isString() ?? false) ? Std.parseInt(this.s) : 0;
	}

	@:to public function toFloat():cpp.Float32 {
		return (this?.isString() ?? false) ? (Std.parseFloat(this.s) : cpp.Float32) : (0.0 : cpp.Float32);
	}

	@:to public function toBool():Bool {
		return (this?.isString() ?? false) && (this.s == "true" || this.s == "1" || this.s == "yes");
	}

	// Read the value back as an object
	@:to public function toObject():JObject {
		return (this?.isObject() ?? false) ? new JObject(this) : new JObject();
	}

	// Read an array as a list of objects
	@:to public function toObjectArray():Array<JObject> {
		if (!(this?.isArray() ?? false)) {
			return [];
		}
		return [
			for (elem in this.array)
				if (elem.isObject()) {
					new JObject(elem);
				} else if (elem.isString()) {
					var o = new JObject();
					o.setString("value", elem.s);
					o;
				} else {
					var o = new JObject();
					o.setArray("array", elem.array);
					o;
				}
		];
	}

	public function isValid():Bool {
		return this != null;
	}

	public function isString():Bool {
		return this?.isString() ?? false;
	}

	public function isObject():Bool {
		return this?.isObject() ?? false;
	}

	public function isArray():Bool {
		return this?.isArray() ?? false;
	}
}

// ---------------------------------------------------------------------------
// Json — the parser
// ---------------------------------------------------------------------------
class Json {
	var text:String;
	var pos:Int;

	public function new(text:String) {
		this.text = text;
		this.pos = 0;
	}

	// The character at the cursor, or "" at end of input
	function cur():String {
		return this.text.charAt(this.pos);
	}

	function skipWs():Void {
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

	function parseObject():JObject {
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
	
	public static function parse(text:String):JObject {
		//  Parse a JSON document, returning its root object
		var p = new Json(text);
		p.skipWs();
		return p.parseObject();
	}
}

// ---------------------------------------------------------------------------
// Optional helpers
// ---------------------------------------------------------------------------
final optionalBool:(JObject, String, Bool) -> Bool = (obj, key, fallback) -> obj.hasKey(key) ? obj[key] : fallback;

final optionalInt:(JObject, String, Int) -> Int = (obj, key, fallback) -> obj.hasKey(key) ? obj[key] : fallback;

final optionalString:(JObject, String, String) -> String = (obj, key, fallback) -> obj.hasKey(key) ? obj[key] : fallback;

final optionalFloat:(JObject, String, cpp.Float32) -> cpp.Float32 = (obj, key, fallback) -> obj.hasKey(key) ? obj[key] : fallback;

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
final parse:(String) -> JObject = (text) -> Json.parse(text);