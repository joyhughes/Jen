#ifndef STATE_MANAGER_HPP
#define STATE_MANAGER_HPP

#include <unordered_map>
#include <memory>
#include <typeindex>
#include <functional>
#include <chrono>
#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "next_element.hpp"

enum probability_distribution {
    PROB_UNIFORM,
    PROB_NORMAL,
    PROB_EXPONENTIAL
};

probability_distribution distribution; 


class ISerializable {
public:
        virtual ~ISerializable() = default;
        virtual nlohmann::json serialize() const = 0;
        virtual bool deserialize(const nlohmann::json& data) = 0;
        virtual std::string get_type_name() const = 0;
        virtual bool is_stateful() const = 0;
};


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

    template<typename T> void register_type(const std::string& type_name) {
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

    template<typename T> nlohmann::json serialize_object(const T& obj) {
        auto type_index = std::type_index(typeid(T));
        auto it = serializers.find(type_index);
        if (it != serializers.end()) {
            auto json_data = it->second(&obj);
            json_data["__type"] = type_names[type_index]; 
            return json_data;
        }

        return nlohmann::json();
    }

    template<typename T> bool deserialize_object(T& obj, const nlohmann::json& data) {
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
            last_modified = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
        }

        return true;
    } catch(...) {
        return false;
    }
};;

    void set_value(float new_value) {
        value = std::clamp(new_value, min, max);
        user_modified = true;
        last_modified = std::chrono::system_clock::now();
    }
}


class enhanced_integrator_float: public SerializableMixin<enhanced_integrator_float> {
public:
    float current_value;
    float starting_value;
    float scale;
    float last_time;
    std::vector<float> history;

    enhanced_integrator_float(float start_val = 0.0f, float scale_val = 1.0f) 
    : current_value(start_val), starting_value(start_val), scale(scale_val), last_time(0.0f) {}

    nlohmann::json serialize() const override {
        nlohmann::json data = {
            {"current_value", current_value},
            {"starting_value", starting_value},
            {"scale", scale},
            {"last_time", last_time}
        }

        if (history.size() > 0) {
            int start_idx = std::max(0, (int)history.size() - 10);
            data["recent_history"] = std::vector<float>(history.begin() + start_idx, history.end());
        }
        return data;
    };

    bool deserialize(const nlohmann::json& data) override {
        try {
            current_value = data.value("current_value", starting_value);
            starting_value = data.value("starting_value", 0.0f);
            scale = data.value("scale", 1.0f);
            last_time = data.value("last_time", 0.0f);


            if (data.contains("recent_history")) {
                history = data["recent_history"].get<std::vector<float>>();
            }

            return true;
        } catch(...) {
            return false;
        }
    }

    void update(float delta_time, float delta_value) {
        current_value += delta_value * scale * delta_time;
        last_time += delta_time;

        history.push_back(current_value);
        if (history.size() > 100) {
            history.erase(history.begin());
        }
    }

    void reset() {
        current_value = starting_value;
        last_time = 0.0f;
        history.clear();
    }
    
}


class enhanced_generator_float : public SerializableMixin<enhanced_generator_float> {
public: 
    bool enabled;
    float probability;
    float param_a, param_b;
    float min_value, max_value;

    std::mt19937 rng;
    uint32_t seed;
    uint64_t generation_count = 0;
    enum probability_distribution { PROB_UNIFORM, PROB_NORMAL, PROB_EXPONENTIAL };
    probability_distribution distribution;


    enhanced_generator_float() : enabled(false), probability(0.001f), param_a(0.0f), param_b(1.0f), min_value(0.0f), max_value(1.0f),
    distribution(PROB_UNIFORM), seed(std::random_device{}())  {
        rng.seed(seed);
    }

    nlohmann::json serialize() const override {
        return {
            {"enabled", enabled},
            {"probability", probability},
            {"param_a", param_a},
            {"param_b", param_b},
            {"min_value", min_value},
            {"max_value", max_value},
            {"distribution", static_cast<int>(distribution)},
            {"seed", seed},
            {"generation_count", generation_count},
            {"rnd_state", serialize_rng_state(rng)}
        };
    }

    bool deserialize(const nlohmann::json& data) override {
        try {
            enabled = data.value("enabled", false);
            probability = data.value("probability", 0.001f);
            param_a = data.value("param_a", 0.0f);
            param_b = data.value("param_b", 1.0f);
            min_value = data.value("min_value", 0.0f);
            max_value = data.value("max_value", 1.0f);
            distribution = static_cast<probability_distribution>(data.value("distribution", 0));
            seed = data.value("seed", 0u);
            generation_count = data.value("generation_count", 0ull);

            if (data.contains("rnd_state")) {
                deserialize_rng_state(rng, data["rnd_state"]);
            } else {
                rng.seed(seed);
            }
            return true;
        } catch(...) {
            return false;
        }
    }

private: 
    std::string serialize_rng_state(const std::mt19937& rng) const {
        std::ostringstream oss;
        oss << rng;
        return oss.str();
    }

    void deserialize_rng_state(std::mt19937& rng, const std::string& state_str) {
        std::istringstream iss(state_str);
        iss >> rng;
    }
}


class enhanced_audio_adder : public SerializableMixin<enhanced_audio_adder> {
public: 
    struct AudioChannel {
        std::string name;
        float weight;
        float sensitivity;
        float smoothed_value = 0.0f;
        float peak_value = 0.0f;
        std::vector<float> recent_values;
    };

    std::vector<AudioChannel> channels;
    float offset;
    float global_sensitivity;

    bool adaptive_mode = false;
    float adaptive_rate = 0.1f;
    std::unordered_map<std::string, float> learned_sensitivities;

    enhanced_audio_adder() : offset(0.0f), global_sensitivity(1.0f) {
        channels = {
            {"volume", 1.0f, 1.0f},
            {"bass", 1.0f, 1.0f},
            {"mid", 1.0f, 1.0f},
            {"high", 1.0f, 1.0f}
        };
    }

    nlohmann::json serialize() const override {
        nlohmann::json channel_data = nlohmann::json::array();
        for (const auto& channel : channels) {
            channel_data.push_back({
                {"name", channel.name},
                {"weight", channel.weight},
                {"sensitivity", channel.sensitivity},
                {"smoothed_value", channel.smoothed_value},
                {"peak_value", channel.peak_value}
            });
        }

        return {
            {"channels", channel_data},
            {"offset", offset},
            {"global_sensitivity", global_sensitivity},
            {"adaptive_mode", adaptive_mode},
            {"adaptation_rate", adaptive_rate},
            {"learned_sensitivities", learned_sensitivities}
        };
    }


    bool deserialize(const nlohmann::json& data) override {
        try {
            channels.clear();            
            for(const auto& channel_data: data["channels"]) {
                AudioChannel channel;
                channel.name = channel_data.value("name", "");
                channel.weight = channel_data.value("weight", 1.0f);
                channel.sensitivity = channel_data.value("sensitivity", 1.0f);
                channel.smoothed_value = channel_data.value("smoothed_value", 0.0f);
                channel.peak_value = channel_data.value("peak_value", 0.0f);
                channels.push_back(channel);
            }
            offset = data.value("offset", 0.0f);
            global_sensitivity = data.value("global_sensitivity", 1.0f);
            adaptive_mode = data.value("adaptive_mode", false);
            adaptive_rate = data.value("adaptation_rate", 0.1f);

            if (data.contains("learned_sensitivities")) {
                learned_sensitivities = data["learned_sensitivities"].get<std::unordered_map<std::string, float>>();
            }

            return true;
        } catch(...) {
            return false;
        }
    };

    bool is_stateful() const override {
        return true;
    }
};

#endif // STATE_MANAGER_HPP