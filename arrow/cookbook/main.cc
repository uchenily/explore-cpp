#include <filesystem>
#include <iostream>

#include <arrow/status.h>
#include <gtest/gtest.h>

#include "common.h"

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();
    if (retval == 0 && HasRecipeOutput()) {
        arrow::Status st = DumpRecipeOutput("recipes_out.arrow");
        if (!st.ok()) {
            std::cerr
                << "Tests ran successfully but failed to dump recipe output: "
                << st << std::endl;
            return -1;
        }
        std::cout << "Created recipe file "
                  << std::filesystem::current_path()
                         .append("recipes_out.arrow")
                         .string()
                  << std::endl;
    }
    return retval;
}
