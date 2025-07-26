#ifndef STATE_MANAGER_HPP
#ifndef STATE_MANAGER_HPP

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <functional>
#include "nlohmann/json.hpp"


class ISerializable {
public:
        virtual ~ISerializable() = default;
        virtual nlohmann::json serialize() const = 0;
        virtual bool deserialize(const nlohmann::json& data) = 0;
        virtual std::string get_type_name() const = 0;
        virtual bool is_stateful() const = 0;
}


class StateRegistry {
private:
    using SerializerMap = std::unordered_map<std::type_index, std::function<nlohmann::json(const void*)>>;
    using DeserializerMap = std::unordered_map<std::type_index, std::function<bool(void*, const nlohmann::json&)>>;
    using TypeNameMap = std::unordered_map<std::type_index, std::string>;


    SerializerMap serializers;
    DeserializerMap deserializers;
    TypeNameMap type_names;

    StateRegistry() = default;

public:
    static StateRegistry& get_instance() {
        static StateRegistry registry;
        return registry;
    }

    template<typename T> register_type(const std::string& type_name) {
        auto type_index = std::type_index(typeid(T));

        type_names[type_index] = type_name;

        // auto generate serializer 
        serializers[type_index] = [](const void* obj) -> nlohmann::json {
            const T* typed_obj = static_cast<const T*>(obj);
            return typed_obj->serialize();
        };

        // auto generate deserializer 

        deserializers[type_index] = [](void* obj, const nlohmann::json& data ) -> bool {
            const T* typed_obj = static_cast<const T*>(obj);
            return typed_obj->deserialize(data);
        };

    }

    template<typename T> serialize_object(const T& obj) {
        auto type_index = std::type_index(typeid(T));
        auto it = serializers.find(type_index);
        if (it != serializers.end()) {
            auto json_data = it->second(&obj);
            json_data["__type"] = type_names[type_index]; 
            return json_data;
        }

        return nlohmann::json();
    }

    template<typename T> deserialize_object(const nlohmann::json& data) {
        auto type_index = std::type_index(typeid(T));
        auto it = deserializers.find(type_index);
        if (it != deserializers.end()) {
            return it->second(&obj, data);
        }
        return false;
    }

    std::string get_type_name(const std::type_index type_index) const{
        auto it = type_names.find(type_index);
        return it != type_names.end() ? it->second : "unknown";
    }

}


template<typename Derived> class SerializableMixin : public ISerializable {
public: 
    std::string get_type_name() const override {
        return StateRegistry::get_instance().get_type_name(std::type_index(typeid(Derived)));
    }

    bool is_stateful() const override {
        return true;
    }
}


#define REGISTER_SERIALIZABLE_TYPE(TypeName) \
    struct TypeName##_Registrar { \
        TypeName##_Registrar() {  \
            StateRegistry::get_instance().register_type<TypeName>(#TypeName); \
        } \
    }; \
    static TypeName##_Registrar registrar;


class enhanced_slider_float : public SerializableMixin<enhanced_slider_float> {
public:
    float value;
    float min, max, default_value;
    float step;
    std::string label, description;
    bool user_modified = false;
    std::chrono::system_clock::time_point last_modified;

    enhanced_slider_float(float def_val = 0.0f, float min_val = 0.0f, float max_val = 1.0f) 
    : value(def_val), min(min_val), max(max_val), default_value(def_val), step(0.1f) {}


    nlohmann::json serialize() const override {
        return {
            {"value", value},
            {"min", min},
            {"max", max},
            {"default_value", default_value},
            {"step", step},
            {"label", label},
            {"description", description},
            {"user_modified", user_modified},
            {"last_modified", std::chrono::duration_cast<std::chrono::seconds>(last_modified.time_since_epoch()).count()}
        }
    }

    bool deserialize(const nlohmann::json&data) override {
        try {
            value = data.value("value", default_value);
            min = data.value("min", min);
            max = data.value("max", max);
            step = data.value("step", 0.1f);
            label = data.value("label", "");
            description = data.value("description", "");
            user_modified = data.value("user_modified", false);

            if (data.contains("last_modified")) {
                auto ms = data["last_modified"].get<int64_t>();
                last_modified = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms))
            }

            return true;
        } catch(...) {
            return false;
        }
    }

    void set_value(float new_value) {
        value = std::clamp(new_value, min, max);
        user_modified = true;
        last_modified = std::chrono::system_clock::now();
    }
}