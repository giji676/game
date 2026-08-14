#pragma once

#include <vector>

enum class InspectType {
    Bool,
    Int,
    Float,
};

struct InspectField {
    const char* name = "";
    InspectType type = InspectType::Bool;
    void* ptr = nullptr;
};

// C++ stand-in for Unity serialized public fields. Scripts and components call
// INSPECT(member) so the editor can build UI from the live field pointers.
class Inspectable {
public:
    const std::vector<InspectField>& inspectFields() const { return fields_; }

protected:
    void expose(const char* name, bool& value) {
        add(name, InspectType::Bool, &value);
    }
    void expose(const char* name, int& value) {
        add(name, InspectType::Int, &value);
    }
    void expose(const char* name, float& value) {
        add(name, InspectType::Float, &value);
    }

private:
    std::vector<InspectField> fields_;

    void add(const char* name, InspectType type, void* ptr) {
        fields_.push_back({name, type, ptr});
    }
};

#define INSPECT(member) expose(#member, member)
