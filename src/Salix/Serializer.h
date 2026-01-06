// src/Salix/Serializer.h

#pragma once
#include <Salix/SalixRPT.h>
#include <string>
#include <fstream>
#include <nlohmann_json/json.hpp>

using json = nlohmann::json;

namespace Salix {

    class Serializer {
        public:

            // --- TO JSON (Save) ---
            static void to_json(json& j, const SalixMarker& smk) {
                j = json {
                    {"index", smk.source_index},
                    {"name", smk.name},
                    {"rel_pos_sec", smk.relative_seconds},
                    {"is_region", smk.is_region},
                    {"duration", smk.duration},
                    {"color", smk.color},
                    {"bpm", smk.context.bpm},
                    {"numerator", smk.context.numerator},
                    {"denominator", smk.context.denominator}
                };
            }
            


            static bool SaveToFile(const std::string& filepath, const ExportQueue& queue) {
                json root;
                root["version"] = "1.0";
                root["total_duration"] = queue.total_duration_seconds;
                root["items"] = json::array();

                for (const auto& item : queue.items) {
                    json j_item;
                    to_json(j_item, item);
                    root["items"].push_back(j_item);
                }

                std::ofstream file(filepath);
                if (!file.is_open()) return false;

                file << root.dump(4);   // 4 space-indentation
                return true;
            }



            // --- FROM JSON (LOAD) ---
            static void from_json(const json& j, SalixMarker& smk) {
                // Use .value() to provide safe defaults if fields are missing
                smk.source_index = j.value("index", -1);
                smk.name = j.value("name", "");
                smk.relative_seconds = j.value("rel_pos_sec", 0.0);
                smk.is_region = j.value("is_region", false);
                smk.duration = j.value("duration", 0.0);
                smk.color = j.value("color", 0);

                // Reconstruct context
                smk.context.bpm = j.value("bpm", 120.0);
                smk.context.numerator = j.value("numerator", 4);
                smk.context.denominator = j.value("denominator", 4);
            }


            static bool LoadFromFile(const std::string& filepath, ExportQueue& queue) {
                std::ifstream file(filepath);
                if (!file.is_open()) return false;

                json root;
                try {
                    file >> root;
                }
                catch (...) {
                    return false;  // JSON Parse Error
                }
                
                queue.items.clear();
                queue.total_duration_seconds = root.value("total_duration", 0.0);

                if (root.contains("items") && root["items"].is_array()) {
                    for (const auto& j_item : root["items"]) {
                        SalixMarker smk;
                        from_json(j_item, smk);
                        queue.items.push_back(smk);
                    }
                }
                return true;
            }
    };
}   // namespace Salix
