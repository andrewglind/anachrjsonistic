#ifndef JSON_H
#define JSON_H

#include <vector>
#include <string>
#include <cstdlib>

namespace json {
    // Forward declarations
    class jvalue;
    class jobject;
    class proxy;

    class jvalue {
    public:
        enum Type { STRING, OBJECT, ARRAY };

        Type type;
        std::string s;
        std::vector<std::string> object_keys;
        std::vector<jvalue> object_vals;
        std::vector<jvalue> array;

        jvalue() : type(STRING) {}

        explicit jvalue(const std::string& v) : type(STRING), s(v) {}

        jvalue* find_key(const std::string& key)
        {
            for (size_t i = 0; i < object_keys.size(); ++i) {
                if (object_keys[i] == key)
                    return &object_vals[i];
            }
            return 0;
        }

        const jvalue* find_key(const std::string& key) const
        {
            for (size_t i = 0; i < object_keys.size(); ++i) {
                if (object_keys[i] == key)
                    return &object_vals[i];
            }
            return 0;
        }

        void set_key(const std::string& key, const jvalue& val)
        {
            for (size_t i = 0; i < object_keys.size(); ++i) {
                if (object_keys[i] == key) {
                    object_vals[i] = val;
                    return;
                }
            }
            object_keys.push_back(key);
            object_vals.push_back(val);
        }

        bool is_string() const { return type == STRING; }
        bool is_object() const { return type == OBJECT; }
        bool is_array()  const { return type == ARRAY;  }
    };

    class jobject {
    public:
        jvalue root;

        jobject()
        {
            root.type = jvalue::OBJECT;
        }

        bool has_key(const std::string& key) const
        {
            return root.find_key(key) != 0;
        }

        void set_string(const std::string& key, const std::string& value)
        {
            root.set_key(key, jvalue(value));
        }

        void set_object(const std::string& key, const jobject& child)
        {
            root.set_key(key, child.root);
        }

        void set_array(const std::string& key, const std::vector<jvalue>& arr)
        {
            jvalue v;
            v.type  = jvalue::ARRAY;
            v.array = arr;
            root.set_key(key, v);
        }

        std::map<std::string, std::string> to_map() const
        {
            std::map<std::string, std::string> out;
            for (size_t i = 0; i < root.object_keys.size(); ++i) {
                if (root.object_vals[i].is_string())
                    out[root.object_keys[i]] = root.object_vals[i].s;
            }
            return out;
        }

        proxy operator[](const std::string& key);
        proxy operator[](const std::string& key) const;
    };

    class proxy {
    public:
        explicit proxy(jvalue* v) : val(v) {}

        proxy operator[](const std::string& key) const
        {
            if (!val || !val->is_object())
                return proxy(0);
            return proxy(val->find_key(key));
        }

        proxy operator[](int index) const
        {
            if (!val || !val->is_array())
                return proxy(0);
            if (index < 0 || index >= (int)val->array.size())
                return proxy(0);
            return proxy(&val->array[index]);
        }

        operator std::string() const
        {
            if (!val || !val->is_string()) return "";
            return val->s;
        }

        operator int() const
        {
            if (!val || !val->is_string()) return 0;
            return ::atoi(val->s.c_str());
        }

        operator double() const
        {
            if (!val || !val->is_string()) return 0.0;
            return ::atof(val->s.c_str());
        }

        operator float() const
        {
            if (!val || !val->is_string()) return 0.0f;
            return (float)::atof(val->s.c_str());
        }

        operator bool() const
        {
            if (!val || !val->is_string()) return false;
            return val->s == "true" || val->s == "1" || val->s == "yes";
        }

        operator jobject () const
        {
            jobject out;
            if (val && val->is_object())
                out.root = *val;
            return out;
        }

        operator std::vector<jobject>() const
        {
            std::vector<jobject> out;
            if (!val || !val->is_array())
                return out;

            for (size_t i = 0; i < val->array.size(); ++i) {
                jobject tmp;
                const jvalue& elem = val->array[i];
                if (elem.is_object()) {
                    tmp.root = elem;
                } else if (elem.is_string()) {
                    tmp.root.set_key("value", jvalue(elem.s));
                } else if (elem.is_array()) {
                    jvalue wrap;
                    wrap.type  = jvalue::ARRAY;
                    wrap.array = elem.array;
                    tmp.root.set_key("array", wrap);
                }
                out.push_back(tmp);
            }
            return out;
        }

        bool is_valid()  const { return val != 0; }
        bool is_string() const { return val && val->is_string(); }
        bool is_object() const { return val && val->is_object(); }
        bool is_array()  const { return val && val->is_array();  }

        jvalue* get() const { return val; }

    private:
        jvalue* val;
    };

    inline proxy jobject::operator[](const std::string& key)
    {
        return proxy(root.find_key(key));
    }

    inline proxy jobject::operator[](const std::string& key) const
    {
        return proxy(const_cast<jvalue*>(root.find_key(key)));
    }

    inline void skip_ws(const char*& p)
    {
        while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
            ++p;
    }

    inline std::string parse_string(const char*& p)
    {
        std::string out;
        ++p; // skip opening "
        while (*p && *p != '"') {
            if (*p == '\\') {
                ++p;
                if      (*p == 'n') out += '\n';
                else if (*p == 't') out += '\t';
                else if (*p == 'r') out += '\r';
                else                out += *p;
            } else {
                out += *p;
            }
            ++p;
        }
        if (*p == '"') ++p; // skip closing "
        return out;
    }

    inline std::string parse_unquoted(const char*& p)
    {
        const char* start = p;
        while (*p && *p != ',' && *p != '}' && *p != ']' &&
               *p != ' '  && *p != '\n' && *p != '\t' && *p != '\r')
            ++p;
        return std::string(start, p - start);
    }

    // Forward declaration so parse_object can call parse_array
    std::vector<jvalue> parse_array(const char*& p);

    inline jobject parse_object(const char*& p)
    {
        jobject obj;
        ++p; // skip '{'
        skip_ws(p);

        while (*p && *p != '}') {
            if (*p != '"') { ++p; continue; } // skip malformed input
            std::string key = parse_string(p);
            skip_ws(p);
            if (*p == ':') ++p;
            skip_ws(p);

            if (*p == '{') {
                obj.set_object(key, parse_object(p));
            } else if (*p == '[') {
                obj.set_array(key, parse_array(p));
            } else if (*p == '"') {
                obj.set_string(key, parse_string(p));
            } else {
                obj.set_string(key, parse_unquoted(p));
            }

            skip_ws(p);
            if (*p == ',') { ++p; skip_ws(p); }
        }

        if (*p == '}') ++p;
        return obj;
    }

    inline std::vector<jvalue> parse_array(const char*& p)
    {
        std::vector<jvalue> arr;
        ++p; // skip '['
        skip_ws(p);

        while (*p && *p != ']') {
            if (*p == '{') {
                jobject child = parse_object(p);
                arr.push_back(child.root);
            } else if (*p == '[') {
                jvalue v;
                v.type  = jvalue::ARRAY;
                v.array = parse_array(p);
                arr.push_back(v);
            } else if (*p == '"') {
                arr.push_back(jvalue(parse_string(p)));
            } else {
                arr.push_back(jvalue(parse_unquoted(p)));
            }

            skip_ws(p);
            if (*p == ',') { ++p; skip_ws(p); }
        }

        if (*p == ']') ++p;
        return arr;
    }

    inline jobject parse(const std::string& text)
    {
        const char* p = text.c_str();
        skip_ws(p);
        return parse_object(p);
    }
}

#endif // JSON_H