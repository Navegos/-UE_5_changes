/******************************************************************************
 * Copyright 2021 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

 // examples/mdl_sdk/dxr/mdl_d3d12/materialx/mdl_generator.h

#ifndef MATERIALX_MDL_GENERATOR_H
#define MATERIALX_MDL_GENERATOR_H

#include "../common.h"

namespace mi {namespace examples { namespace mdl_d3d12 { namespace materialx
{
    class Mdl_generator_result
    {
    public:
        std::vector<std::string> materialx_file_name;
        std::vector<std::string> generated_mdl_code;
    };

    // --------------------------------------------------------------------------------------------

    /// Basic MDL code-gen from MaterialX.
    /// A more sophisticated supports will be needed for full function support.
    class Mdl_generator
    {
    public:
        /// Constructor.
        explicit Mdl_generator();

        /// Destructor.
        ~Mdl_generator() = default;

        /// add mtlx library that is required for code generation.
        /// TODO: - find a way to resolve this dependencies automatically.
        bool add_dependency(const std::string& mtlx_library);

        /// set the main mtlx file of the material to generate code from.
        bool set_source(const std::string& mtlx_material);

        /// generate mdl code
        /// TODO: - handle multiple materials and functions
        ///       - handle relative resources
        bool generate(Mdl_generator_result& inout_result) const;

    private:
        std::vector<std::string> m_dependencies;
        std::string m_mtlx_source;
    };

}}}}
#endif
