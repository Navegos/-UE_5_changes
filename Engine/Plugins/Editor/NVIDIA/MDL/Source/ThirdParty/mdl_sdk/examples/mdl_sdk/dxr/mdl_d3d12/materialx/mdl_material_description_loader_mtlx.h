/******************************************************************************
 * Copyright 2021 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

 // examples/mdl_sdk/dxr/mdl_d3d12/materialx/mdl_material_description_loader_mtlx.h

#ifndef MATERIALX_MDL_MATERIAL_DESCRIPTION_LOADER_MTLX_H
#define MATERIALX_MDL_MATERIAL_DESCRIPTION_LOADER_MTLX_H

#include "../common.h"
#include "../mdl_material_description.h"

namespace mi {namespace examples { namespace mdl_d3d12 { namespace materialx
{
    /// loader to generate MDL from MaterialX
    /// see IMdl_material_description_loader for documentation
    class Mdl_material_description_loader_mtlx : public IMdl_material_description_loader
    {
    public:
        bool match_gltf_name(const std::string& gltf_name) const final;

        std::string generate_mdl_source_code(
            const std::string& gltf_name,
            const std::string& scene_directory) const final;

        std::string get_scene_name_prefix() const final { return "[MTLX]"; }

        bool supports_reload() const final { return true; }

        size_t get_file_type_count() const final;
        std::string get_file_type_extension(size_t index) const final;
        std::string get_file_type_description(size_t index) const final;
    };

}}}}
#endif
