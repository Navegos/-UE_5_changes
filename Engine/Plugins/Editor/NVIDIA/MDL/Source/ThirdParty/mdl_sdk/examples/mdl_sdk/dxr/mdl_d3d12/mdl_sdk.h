/******************************************************************************
 * Copyright 2021 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

 // examples/mdl_sdk/dxr/mdl_d3d12/mdl_sdk.h

#ifndef MDL_D3D12_MDL_SDK_H
#define MDL_D3D12_MDL_SDK_H

#include "common.h"
#include "example_shared.h"

namespace mi { namespace examples { namespace mdl_d3d12
{
    class Base_application;
    class Mdl_transaction;
    class Mdl_material_library;
    enum class Texture_dimension;

    // --------------------------------------------------------------------------------------------

    enum class Mdl_resource_kind
    {
        Texture, // includes 2D and 3D textures
        // Light_profile,
        // Bsdf_measurement,
        _Count
    };

    // --------------------------------------------------------------------------------------------

    /// Set of GPU resources that belong to one MDL resource
    struct Mdl_resource_set
    {
        struct Entry
        {
            explicit Entry()
                : resource(nullptr)
                , udim_u(0)
                , udim_v(0)
            {}

            Resource* resource; // texture or buffer
            int32_t udim_u;     // u-coordinate of the lower left corner of the tile
            int32_t udim_v;     // v-coordinate of the lower left corner of the tile
        };

        explicit Mdl_resource_set()
            : entries()
            , is_udim_tiled(false)
            , udim_u_min(0)
            , udim_u_max(0)
            , udim_v_min(0)
            , udim_v_max(0)
        {
        }

        std::vector<Entry> entries; // tile resources

        bool is_udim_tiled;         // true UDIM texture 2D, false otherwise
        int32_t udim_u_min;         // u-coordinate of the tile most left bottom
        int32_t udim_u_max;         // v-coordinate of the tile most right top
        int32_t udim_v_min;         // u-coordinate of the tile most left bottom
        int32_t udim_v_max;         // v-coordinate of the tile most right top

        int32_t get_udim_width() const
        {
            return (udim_u_max + 1 - udim_u_min);
        }

        int32_t get_tile_count() const
        {
            return (udim_u_max + 1 - udim_u_min) * (udim_v_max + 1 - udim_v_min);
        }

        size_t compute_linear_udim_index(size_t entry_index)
        {
            int32_t u = entries[entry_index].udim_u - udim_u_min;
            int32_t v = entries[entry_index].udim_v - udim_v_min;
            return u + v * get_udim_width();
        }
    };

    // --------------------------------------------------------------------------------------------

    struct Mdl_resource_assignment
    {
        explicit Mdl_resource_assignment(Mdl_resource_kind kind)
            : kind(kind)
            , resource_name("")
            , runtime_resource_id(0)
            , data(nullptr)
        {
        }

        // type of resource
        Mdl_resource_kind kind;

        // DB name of the resource
        std::string resource_name;

        // Texture dimension in case of texture resources.
        Texture_dimension dimension;

        // ID generated by the SDK or the ITarget_resource_callback.
        // Is passed to the HLSL mdl renderer runtime.
        uint32_t runtime_resource_id;

        // textures and buffers
        Mdl_resource_set* data;
    };

    // --------------------------------------------------------------------------------------------

    /// Information passed to GPU for mapping id requested in the runtime functions to buffer
    /// views of the corresponding type.
    struct Mdl_resource_info
    {
        // index into the tex2d, tex3d, ... buffers, depending on the type requested
        uint32_t gpu_resource_array_start;

        // number resources (e.g. UDIM tiles) that belong to this resource
        uint32_t gpu_resource_array_size;

        // coordinate of the left bottom most UDIM tile (also bottom left corner)
        int32_t gpu_resource_udim_u_min;
        int32_t gpu_resource_udim_v_min;

        // in case of UDIM textures,  required to calculate a linear index (u + v * width
        uint32_t gpu_resource_udim_width;
    };

    // --------------------------------------------------------------------------------------------

    class Mdl_sdk
    {
    public:
        /// A set of options that controls the MDL SDK and the code generation in global manner.
        /// These options can be bound to the GUI to be adjusted by the user at runtime.
        struct Options
        {
            bool use_class_compilation;
            bool fold_all_bool_parameters;
            bool fold_all_enum_parameters;

            bool enable_shader_cache; // note, this is not really an SDK option but fits here
        };

        // ----------------------------------------------------------------------------------------

        explicit Mdl_sdk(Base_application* app);
        virtual ~Mdl_sdk();

        // true if the MDL SDK was initialized correctly
        bool is_valid() const { return m_valid; }

        /// logs errors, warnings, infos, ... and returns true in case the was NO error
        /// meant to be called using the SRC macro: 'log_messages(context, SRC)'
        bool log_messages(
            const std::string& message,
            const mi::neuraylib::IMdl_execution_context* context,
            const std::string& file = "",
            int line = 0);

        mi::neuraylib::INeuray& get_neuray() { return *m_neuray; }
        mi::neuraylib::IMdl_configuration& get_config() { return *m_config; }
        mi::neuraylib::IDatabase& get_database() { return *m_database; }
        mi::neuraylib::IMdl_evaluator_api& get_evaluator() { return *m_evaluator_api; }
        mi::neuraylib::IMdl_factory& get_factory() { return *m_mdl_factory; }
        mi::neuraylib::IImage_api& get_image_api() { return *m_image_api; }
        mi::neuraylib::IMdl_impexp_api& get_impexp_api() { return *m_mdl_impexp_api; }
        mi::neuraylib::IMdl_backend& get_backend() { return *m_hlsl_backend; }

        /// Updates the mdl search paths.
        /// These include the default admin and user space paths, the example search path,
        /// and if available the application folder.
        /// While the first two are best practice, the other ones make it easier to copy
        /// the example binaries without losing the referenced example MDL modules.
        /// Additionally, the current scene path is added to allow per scene materials and this
        /// why the search paths are reconfigured after loading a new scene.
        void reconfigure_search_paths();

        // Creates a new execution context. At least one per thread is required.
        // This means you can share the context for multiple calls from the same thread.
        // However, sharing is not required. Creating a context for each call is valid too but
        // slightly more expensive.
        // Use a neuray handle to hold the pointer returned by this function.
        mi::neuraylib::IMdl_execution_context* create_context();

        /// access point to the database
        Mdl_transaction& get_transaction() { return *m_transaction; }

        /// keeps all materials that are loaded by the application
        Mdl_material_library* get_library() { return m_library; }

        /// A set of options that controls the MDL SDK and the code generation in global manner.
        /// These options can be bound to the GUI to be adjusted by the user at runtime.
        Options& get_options() { return m_mdl_options; }

    private:
        Base_application* m_app;

        mi::base::Handle<mi::neuraylib::INeuray> m_neuray;
        mi::base::Handle<mi::neuraylib::IMdl_configuration> m_config;
        mi::base::Handle<mi::neuraylib::IDatabase> m_database;
        mi::base::Handle<mi::neuraylib::IImage_api> m_image_api;
        mi::base::Handle<mi::neuraylib::IMdl_factory> m_mdl_factory;
        mi::base::Handle<mi::neuraylib::IMdl_backend> m_hlsl_backend;
        mi::base::Handle<mi::neuraylib::IMdl_impexp_api> m_mdl_impexp_api;
        mi::base::Handle<mi::neuraylib::IMdl_evaluator_api> m_evaluator_api;
        Mdl_transaction* m_transaction;
        Mdl_material_library* m_library;
        Options m_mdl_options;
        bool m_valid;
    };

    // --------------------------------------------------------------------------------------------

    class Mdl_transaction
    {
        // make sure there is only one transaction
        friend class Mdl_sdk;

        explicit Mdl_transaction(Mdl_sdk* sdk);
    public:
        virtual ~Mdl_transaction();

        // runs an operation on the database.
        // concurrent calls are executed in sequence using a lock.
        template<typename TRes>
        TRes execute(std::function<TRes(mi::neuraylib::ITransaction* t)> action)
        {
            std::lock_guard<std::mutex> lock(m_transaction_mtx);
            return action(m_transaction.get());
        }

        template<>
        void execute<void>(std::function<void(mi::neuraylib::ITransaction* t)> action)
        {
            std::lock_guard<std::mutex> lock(m_transaction_mtx);
            action(m_transaction.get());
        }

        // locked database access function
        template<typename TIInterface>
        const TIInterface* access(const char* db_name)
        {
            return execute<const TIInterface*>(
                [&](mi::neuraylib::ITransaction* t)
            {
                return t->access<TIInterface>(db_name);
            });
        }

        // locked database access function
        template<typename TIInterface>
        TIInterface* edit(const char* db_name)
        {
            return execute<TIInterface*>(
                [&](mi::neuraylib::ITransaction* t)
            {
                return t->edit<TIInterface>(db_name);
            });
        }

        // locked database create function
        template<typename TIInterface>
        TIInterface* create(const char* type_name)
        {
            return execute<TIInterface*>(
                [&](mi::neuraylib::ITransaction* t)
            {
                return t->create<TIInterface>(type_name);
            });
        }

        // locked database store function
        template<typename TIInterface>
        mi::Sint32 store(
            TIInterface* db_element,
            const char* name)
        {
            return execute<mi::Sint32>(
                [&](mi::neuraylib::ITransaction* t)
            {
                return t->store(db_element, name);
            });
        }

        // locked database commit function.
        // For that, all handles to neuray objects have to be released.
        // Initializes for further actions afterwards.
        void commit();

        mi::neuraylib::ITransaction* get() { return m_transaction.get(); }

    private:
        mi::base::Handle<mi::neuraylib::ITransaction> m_transaction;
        std::mutex m_transaction_mtx;
        Mdl_sdk* m_sdk;
    };

}}} // mi::examples::mdl_d3d12
#endif
