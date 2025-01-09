
#pragma once
#include <twl/fs/fs_File.hpp>

namespace twl::fs {

    class FileFormat {
        public:
            virtual Result ReadValidateFrom(File &rf) = 0;
            virtual Result ReadAllFrom(File &rf) = 0;
            virtual Result WriteTo(File &wf) = 0;

            inline Result ReadFrom(File &rf) {
                TWL_R_TRY(this->ReadValidateFrom(rf));
                TWL_R_TRY(this->ReadAllFrom(rf));
                TWL_R_SUCCEED();
            }
    };

}
