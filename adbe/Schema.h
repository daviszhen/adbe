#ifndef __SCHEMA_H__
#define __SCHEMA_H__

#include <list>
#include <unordered_map>

enum FieldType {
    INVALID_FIELD_TYPE=-1,
    INTEGER = 0,
    VARCHAR = 1
};

struct FieldInfo {
    FieldType type;
    int length;
    
    FieldInfo(FieldType type = INVALID_FIELD_TYPE, int length = -1) {
        this->type = type;
        this->length = length;
    }

    std::string toString() {
        if (type == VARCHAR) {
            return "varchar(" + std::to_string(length) + ")";
        }
        else if(type == INTEGER){
            return "int";
        }
        else {
            return "invalid_type";
        }
    }
};

//记录的schema. 字段的名称，类型，varchar字段的长度
class Schema {
public:
    Schema(){}

    //增加一个新字段. INTEGER的长度值是无关的。
    void addField(const std::string& fldname, FieldType type, int length) {
        _fields.push_back(fldname);
        info[fldname]=FieldInfo(type, length);
    }

    //添加integer字段
    void addIntField(const std::string& fldname) {
        addField(fldname, INTEGER, 0);
    }

    //增加varchar类型。varchar(length)
    void addStringField(const std::string& fldname, int length) {
        addField(fldname, VARCHAR, length);
    }

    void add(const std::string& fldname, Schema& sch) {
        FieldType type = sch.type(fldname);
        int length = sch.length(fldname);
        addField(fldname, type, length);
    }

    void addAll(Schema& sch) {
        for (std::string fldname : sch.fields())
            add(fldname, sch);
    }

    const std::list<std::string>& fields()const {
        return _fields;
    }

    bool hasField(const std::string& fldname) {
        return std::find(_fields.begin(),_fields.end(),fldname) != _fields.end();
    }

    FieldType type(const std::string& fldname) {
        return info[fldname].type;
    }

    int length(const std::string& fldname) {
        return info[fldname].length;
    }

    std::string toString() {
        std::string ans("Schema:");
        for (auto& t : _fields) {
            ans += t + "," + info[t].toString() + ",";
        }
        return ans;
    }
private:
	std::list<std::string> _fields;
	std::unordered_map<std::string, FieldInfo> info;
};

#endif // !__SCHEMA_H__
