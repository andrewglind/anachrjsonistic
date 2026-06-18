#include "Json.h"

namespace json {

	jobject parse(const std::string& text) {
	Json* p = new Json(text);
	p->skipWs();
	jobject _ret1 = p->parseObject();
	delete p;
	return _ret1;
	}

	jvalue::jvalue() {
	this->type = JType_::Str;
	this->s = "";
	this->objectKeys.clear();
	this->objectVals.clear();
	this->array.clear();
	}

	jvalue* jvalue::ofString(const std::string& v) {
	jvalue* j = new jvalue();
	j->type = JType_::Str;
	j->s = v;
	return j;
	}

	jvalue* jvalue::ofArray(const std::vector<jvalue*>& a) {
	jvalue* j = new jvalue();
	j->type = JType_::Arr;
	j->array = a;
	return j;
	}

	jvalue* jvalue::findKey(const std::string& key) {
	for (int _i1 = 0; _i1 < this->objectKeys.size(); ++_i1) {
		if (this->objectKeys[_i1] == key) {
			return this->objectVals[_i1];
		}
	}
	return NULL;
	}

	void jvalue::setKey(const std::string& key, jvalue* val) {
	for (int _i2 = 0; _i2 < this->objectKeys.size(); ++_i2) {
		if (this->objectKeys[_i2] == key) {
			size_t _ix3 = (size_t)(_i2);
			if (_ix3 >= this->objectVals.size()) this->objectVals.resize(_ix3 + 1);
			this->objectVals[_ix3] = val;
			return;
		}
	}
	this->objectKeys.push_back(key);
	this->objectVals.push_back(val);
	}

	bool jvalue::isString() {
	return this->type == JType_::Str;
	}

	bool jvalue::isObject() {
	return this->type == JType_::Obj;
	}

	bool jvalue::isArray() {
	return this->type == JType_::Arr;
	}

	jobject::jobject(jvalue* root) {
	if (root == NULL) {
		this->__this = new jvalue();
		this->__this->type = JType_::Obj;
	} else {
		this->__this = root;
	}
	}

	jvalue* jobject::root() {
	return this->__this;
	}

	bool jobject::hasKey(const std::string& key) {
	return this->__this->findKey(key) != NULL;
	}

	void jobject::setString(const std::string& key, const std::string& value) {
	this->__this->setKey(key, jvalue::ofString(value));
	}

	void jobject::setObject(const std::string& key, jobject child) {
	this->__this->setKey(key, child.root());
	}

	void jobject::setArray(const std::string& key, const std::vector<jvalue*>& arr) {
	this->__this->setKey(key, jvalue::ofArray(arr));
	}

	std::map<std::string, std::string> jobject::toMap() {
	std::map<std::string, std::string> out = std::map<std::string, std::string>();
	for (int _i1 = 0; _i1 < this->__this->objectKeys.size(); ++_i1) {
		if (this->__this->objectVals[_i1]->isString()) {
			out[this->__this->objectKeys[_i1]] = this->__this->objectVals[_i1]->s;
		}
	}
	return out;
	}

	proxy jobject::get(const std::string& key) {
	return proxy(this->__this->findKey(key));
	}

	proxy::proxy(jvalue* v) {
	this->__this = v;
	}

	proxy proxy::key(const std::string& k) {
	if (this->__this == NULL || !this->__this->isObject()) {
		return proxy(NULL);
	}
	return proxy(this->__this->findKey(k));
	}

	proxy proxy::index(int i) {
	if (this->__this == NULL || !this->__this->isArray()) {
		return proxy(NULL);
	}
	if (i < 0 || i >= this->__this->array.size()) {
		return proxy(NULL);
	}
	return proxy(this->__this->array[i]);
	}

	std::string proxy::toStr() {
	if (this->__this == NULL || !this->__this->isString()) {
		return "";
	}
	return this->__this->s;
	}

	int proxy::toInt() {
	if (this->__this == NULL || !this->__this->isString()) {
		return 0;
	}
	return (int)strtol(this->__this->s.c_str(), NULL, 0);
	}

	float proxy::toFloat() {
	if (this->__this == NULL || !this->__this->isString()) {
		return ((float) 0.0);
	}
	return ((float) atof(this->__this->s.c_str()));
	}

	bool proxy::toBool() {
	if (this->__this == NULL || !this->__this->isString()) {
		return false;
	}
	return this->__this->s == "true" || this->__this->s == "1" || this->__this->s == "yes";
	}

	jobject proxy::toObject() {
	if (this->__this != NULL && this->__this->isObject()) {
		return jobject(this->__this);
	}
	return jobject();
	}

	std::vector<jobject> proxy::toObjectArray() {
	std::vector<jobject> out;
	if (this->__this == NULL || !this->__this->isArray()) {
		return out;
	}
	for (int _i1 = 0; _i1 < this->__this->array.size(); ++_i1) {
		jvalue* elem = this->__this->array[_i1];
		if (elem->isObject()) {
			out.push_back(jobject(elem));
		} else {
			if (elem->isString()) {
				jobject o = jobject();
				o.setString("value", elem->s);
				out.push_back(o);
			} else {
				if (elem->isArray()) {
					jobject o = jobject();
					o.setArray("array", elem->array);
					out.push_back(o);
				}
			}
		}
	}
	return out;
	}

	bool proxy::isValid() {
	return this->__this != NULL;
	}

	bool proxy::isString() {
	return this->__this != NULL && this->__this->isString();
	}

	bool proxy::isObject() {
	return this->__this != NULL && this->__this->isObject();
	}

	bool proxy::isArray() {
	return this->__this != NULL && this->__this->isArray();
	}

	Json::Json(const std::string& text) {
	this->text = text;
	this->pos = 0;
	}

	std::string Json::cur() {
	return this->text.substr(this->pos, 1);
	}

	void Json::skipWs() {
	std::string c = cur();
	while (c == " " || c == "\n" || c == "\t" || c == "\r") {
		this->pos++;
		c = cur();
	}
	}

	std::string Json::parseString() {
	std::string out = "";
	this->pos++;
	std::string c = cur();
	while (c != "" && c != "\"") {
		if (c == "\\") {
			this->pos++;
			std::string e = cur();
			if (e == "") {
				break;
			}
			if (e == "n") {
				out += "\n";
			} else {
				if (e == "t") {
					out += "\t";
				} else {
					if (e == "r") {
						out += "\r";
					} else {
						out += e;
					}
				}
			}
			this->pos++;
		} else {
			out += c;
			this->pos++;
		}
		c = cur();
	}
	if (cur() == "\"") {
		this->pos++;
	}
	return out;
	}

	std::string Json::parseUnquoted() {
	int start = this->pos;
	std::string c = cur();
	while (c != "" && c != "," && c != "}" && c != "]" && c != " " && c != "\n" && c != "\t" && c != "\r") {
		this->pos++;
		c = cur();
	}
	std::string _s1 = this->text;
	int _a2 = (int)(start); if (_a2 < 0) _a2 = 0; if ((size_t)_a2 > _s1.size()) _a2 = (int)_s1.size();
	int _b3 = (int)(this->pos); if (_b3 < 0) _b3 = 0; if ((size_t)_b3 > _s1.size()) _b3 = (int)_s1.size();
	if (_a2 > _b3) { int t = _a2; _a2 = _b3; _b3 = t; }
	std::string _sub4 = _s1.substr((size_t)_a2, (size_t)(_b3 - _a2));
	return _sub4;
	}

	jobject Json::parseObject() {
	jobject obj = jobject();
	this->pos++;
	skipWs();
	std::string c = cur();
	while (c != "" && c != "}") {
		if (c != "\"") {
			this->pos++;
			c = cur();
			continue;
		}
		std::string key = parseString();
		skipWs();
		if (cur() == ":") {
			this->pos++;
		}
		skipWs();
		std::string d = cur();
		if (d == "{") {
			obj.setObject(key, parseObject());
		} else {
			if (d == "[") {
				obj.setArray(key, parseArray());
			} else {
				if (d == "\"") {
					obj.setString(key, parseString());
				} else {
					obj.setString(key, parseUnquoted());
				}
			}
		}
		skipWs();
		if (cur() == ",") {
			this->pos++;
			skipWs();
		}
		c = cur();
	}
	if (cur() == "}") {
		this->pos++;
	}
	return obj;
	}

	std::vector<jvalue*> Json::parseArray() {
	std::vector<jvalue*> arr;
	this->pos++;
	skipWs();
	std::string c = cur();
	while (c != "" && c != "]") {
		if (c == "{") {
			jobject child = parseObject();
			arr.push_back(child.root());
		} else {
			if (c == "[") {
				arr.push_back(jvalue::ofArray(parseArray()));
			} else {
				if (c == "\"") {
					arr.push_back(jvalue::ofString(parseString()));
				} else {
					arr.push_back(jvalue::ofString(parseUnquoted()));
				}
			}
		}
		skipWs();
		if (cur() == ",") {
			this->pos++;
			skipWs();
		}
		c = cur();
	}
	if (cur() == "]") {
		this->pos++;
	}
	return arr;
	}

}
