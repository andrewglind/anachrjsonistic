#ifndef JSON_H
#define JSON_H

#include "StdAfx.h"

namespace json {

	class proxy;

	struct JType_ {
		enum Enum {
			Str,
			Obj,
			Arr
		};
	};
	typedef JType_::Enum JType;

	class jvalue {
	public:
		jvalue();
		virtual ~jvalue() {}
		static jvalue* ofString(const std::string& v);
		static jvalue* ofArray(const std::vector<jvalue*>& a);
		jvalue* findKey(const std::string& key);
		void setKey(const std::string& key, jvalue* val);
		bool isString();
		bool isObject();
		bool isArray();
		JType type;
		std::string s;
		std::vector<std::string> objectKeys;
		std::vector<jvalue*> objectVals;
		std::vector<jvalue*> array;
	};

	class jobject {
	public:
		jobject(jvalue* root = NULL);
		~jobject() {}
		jvalue* root();
		bool hasKey(const std::string& key);
		void setString(const std::string& key, const std::string& value);
		void setObject(const std::string& key, jobject child);
		void setArray(const std::string& key, const std::vector<jvalue*>& arr);
		std::map<std::string, std::string> toMap();
		proxy get(const std::string& key);
		proxy operator[](const std::string& key);
	protected:
		jvalue* __this;
	};

	class proxy {
	public:
		proxy(jvalue* v);
		~proxy() {}
		proxy key(const std::string& k);
		proxy operator[](const std::string& k) { return key(k); }
		proxy index(int i);
		proxy operator[](int i) { return index(i); }
		std::string toStr();
		operator std::string() { return toStr(); }
		int toInt();
		operator int() { return toInt(); }
		float toFloat();
		operator float() { return toFloat(); }
		bool toBool();
		operator bool() { return toBool(); }
		jobject toObject();
		operator jobject() { return toObject(); }
		std::vector<jobject> toObjectArray();
		operator std::vector<jobject>() { return toObjectArray(); }
		bool isValid();
		bool isString();
		bool isObject();
		bool isArray();
	protected:
		jvalue* __this;
	};

	class Json {
	public:
		Json(const std::string& text);
		virtual ~Json() {}
		std::string cur();
		void skipWs();
		std::string parseString();
		std::string parseUnquoted();
		jobject parseObject();
		std::vector<jvalue*> parseArray();
	protected:
		std::string text;
		int pos;
	};

	inline proxy jobject::operator[](const std::string& key) { return get(key); }

	jobject parse(const std::string& text);

}

#endif
