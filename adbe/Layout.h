#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include "page.h"
#include "Schema.h"

#include <unordered_map>

//记录的物理结构
class Layout {
public:
    Layout(const Schema& schema){
        this->scha = schema;
        int pos = sizeof(int); // leave space for the empty/inuse flag
        for (const std::string fldname : schema.fields()) {
            offsets[fldname] = pos;
            pos += lengthInBytes(fldname);
        }
        slotsize = pos;
    }

    Layout(const Schema& schema, 
        const std::unordered_map<const std::string, int>& offsets, 
        int slotsize){
        this->scha = schema;
        for (auto pr : this->offsets) {
            this->offsets[pr.first] = pr.second;
        }
        this->slotsize = slotsize;
    }

    Schema& schema() {
        return this->scha;
    }

    const Schema& schema()const {
        return this->scha;
    }
    
    int offset(const std::string& fldname) {
        return offsets[fldname];
    }

    int slotSize() {
        return slotsize;
    }

    int lengthInBytes(const std::string& fldname) {
        FieldType fldtype = this->scha.type(fldname);
        if (fldtype == INTEGER)
            return sizeof(int);
        else // fldtype == VARCHAR
            return Page::maxLength(this->scha.length(fldname));
    }
private:
	Schema scha;
	std::unordered_map<std::string, int> offsets;
	int slotsize;
};
#endif // !__LAYOUT_H__
