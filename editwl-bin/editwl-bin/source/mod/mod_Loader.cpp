#include <mod/mod_Loader.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>

namespace etwl::mod {

    namespace {

        std::vector<Module> g_Modules;

        #define QUOTE(name) #name
        #define STR(macro) QUOTE(macro)

        #define ETWL_MOD_TRY_RESOLVE_MOD_SYMBOL(symbol, type, field) { \
            auto symbol_fn = (type)lib->resolve(STR(symbol)); \
            if(symbol_fn == nullptr) { \
                return ResultInvalidModuleSymbols; \
            } \
            out_mod.symbols.field = symbol_fn; \
        }

        twl::Result TryLoadModule(const QString &path, Module &out_mod) {
            auto lib = std::make_shared<QLibrary>(path);
            if(lib->load()) {
                ETWL_MOD_TRY_RESOLVE_MOD_SYMBOL(ETWL_MOD_INITIALIZE_SYMBOL, InitializeFunction, init_fn);
                ETWL_MOD_TRY_RESOLVE_MOD_SYMBOL(ETWL_MOD_TRY_HANDLE_COMMAND_SYMBOL, TryHandleCommandFunction, try_handle_cmd_fn);
                ETWL_MOD_TRY_RESOLVE_MOD_SYMBOL(ETWL_MOD_TRY_HANDLE_INPUT_SYMBOL, TryHandleInputFunction, try_handle_input_fn);

                if(!out_mod.symbols.init_fn(&out_mod.meta)) {
                    return ResultModuleInitializationFailure;
                }

                RegisterResultDescriptionTable(out_mod.meta.rc_table, out_mod.meta.rc_table_size);

                out_mod.dyn_lib = std::move(lib);
                TWL_R_SUCCEED();
            }
            else {
                qWarning() << "Error loading module: " << lib->errorString();
                return ResultModuleLoadError;
            }
        }

    }

    void LoadModules() {
        const auto cwd = QDir(QCoreApplication::applicationDirPath());
        const auto modules_dir = cwd.filePath(ETWL_MODULES_DIR);

        QDirIterator iter(modules_dir, QDir::Files);
        while(iter.hasNext()) {
            const auto mod_file = iter.next();

            Module mod;
            const auto rc = TryLoadModule(mod_file, mod);
            if(rc.IsSuccess()) {
                g_Modules.push_back(std::move(mod));
            }
            else {
                qWarning() << "File '" << mod_file << "' is not a valid module: " << FormatResult(rc);
            }
        }
    }

    std::vector<Module> &GetModules() {
        return g_Modules;
    }

}
