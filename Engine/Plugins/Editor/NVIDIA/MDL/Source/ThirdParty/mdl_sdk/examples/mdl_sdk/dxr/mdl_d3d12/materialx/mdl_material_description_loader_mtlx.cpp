/******************************************************************************
 * Copyright 2021 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#include "mdl_material_description_loader_mtlx.h"
#include "mdl_generator.h"
#include <algorithm>

namespace mi {namespace examples { namespace mdl_d3d12 { namespace materialx
{

bool Mdl_material_description_loader_mtlx::match_gltf_name(const std::string& gltf_name) const
{
    return mi::examples::strings::ends_with(gltf_name, ".mtlx");
}

// ------------------------------------------------------------------------------------------------

std::string Mdl_material_description_loader_mtlx::generate_mdl_source_code(
    const std::string& gltf_name,
    const std::string& scene_directory) const
{
    mdl_d3d12::materialx::Mdl_generator mtlx2mdl;
    mdl_d3d12::materialx::Mdl_generator_result result;

    // currently dependencies are added manually
    // until a discover mechanism is implemented
    std::string mx_repo = mi::examples::io::get_executable_folder() + "/autodesk_materialx";
    if (mx_repo.empty())
    {
        log_error("MATERIALX_REPOSITORY environment variable is not set. "
            "Currently static dependencies can not be resolved. "
            "Continuing with gltf material parameters.", SRC);
        return "";
    }

    bool valid = true;
    valid &= mtlx2mdl.add_dependency(mx_repo + "/libraries/bxdf/standard_surface.mtlx");
    valid &= mtlx2mdl.add_dependency(mx_repo + "/libraries/bxdf/usd_preview_surface.mtlx");

    // set the material file to load
    std::string mtlx_material_file = mi::examples::io::is_absolute_path(gltf_name)
        ? gltf_name
        : scene_directory + "/" + gltf_name;

    // set the materials main source file
    valid &= mtlx2mdl.set_source(mtlx_material_file);

    // generate the mdl code
    try
    {
        valid &= mtlx2mdl.generate(result);
    }
    catch (const std::exception & ex)
    {
        log_error("Generated MDL from materialX crashed: " + gltf_name, ex, SRC);
        return "";
    }

    if (!valid)
    {
        log_error("Generated MDL from materialX: " + gltf_name, SRC);
        return "";
    }

    // dump the mdl for debugging only
#if DEBUG
    {
        size_t pos = gltf_name.find_last_of('/');
        std::string file_name = pos == std::string::npos
            ? gltf_name + ".mdl"
            : gltf_name.substr(pos + 1) + ".mdl";

        file_name = mi::examples::io::get_executable_folder() + "/" + file_name;
        auto file = std::ofstream();
        file.open(file_name, std::ofstream::out | std::ofstream::trunc);
        if (file.is_open())
        {
            file << result.generated_mdl_code[0];
            file.close();
        }
    }
#endif
    // return the first generated code segment.
    return result.generated_mdl_code[0];
}

// ------------------------------------------------------------------------------------------------

size_t Mdl_material_description_loader_mtlx::get_file_type_count() const
{
    return 1;
}

// ------------------------------------------------------------------------------------------------

std::string Mdl_material_description_loader_mtlx::get_file_type_extension(size_t index) const
{
    switch (index)
    {
        case 0: return "mtlx";
        default: return "";
    }
}

// ------------------------------------------------------------------------------------------------
std::string Mdl_material_description_loader_mtlx::get_file_type_description(size_t index) const
{
    switch (index)
    {
        case 0: return "MaterialX";
        default: return "";
    }
}

}}}}
