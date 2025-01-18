#ifndef ARROW_COOKBOOK_COMMON_H
#define ARROW_COOKBOOK_COMMON_H

#include <arrow/result.h>
#include <arrow/status.h>

#include <sstream>
#include <string>

#define ARROW_STRINGIFY(x) #x
#define ARROW_CONCAT(x, y) x##y

#define ARROW_ASSIGN_OR_RAISE_NAME(x, y) ARROW_CONCAT(x, y)

#define ASSERT_OK(expr)                                                        \
    for (const ::arrow::Status &_st                                            \
         = ::arrow::internal::GenericToStatus((expr));                         \
         !_st.ok();)                                                           \
    FAIL() << "'" ARROW_STRINGIFY(expr) "' failed with " << _st.ToString()

#define ASSIGN_OR_HANDLE_ERROR_IMPL(handle_error, status_name, lhs, rexpr)     \
    auto &&status_name = (rexpr);                                              \
    handle_error(status_name.status());                                        \
    lhs = std::move(status_name).ValueOrDie();

#define ASSERT_OK_AND_ASSIGN(lhs, rexpr)                                       \
    ASSIGN_OR_HANDLE_ERROR_IMPL(                                               \
        ASSERT_OK,                                                             \
        ARROW_ASSIGN_OR_RAISE_NAME(_error_or_value, __COUNTER__),              \
        lhs,                                                                   \
        rexpr);

inline std::stringstream rout;

void                       StartRecipe(const std::string &recipe_name);
void                       EndRecipe(const std::string &recipe_name);
arrow::Status              DumpRecipeOutput(const std::string &output_filename);
bool                       HasRecipeOutput();
arrow::Result<std::string> FindTestDataFile(const std::string &test_data_name);

#endif // ARROW_COOKBOOK_COMMON_H
