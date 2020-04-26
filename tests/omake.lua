project = CreateProject()

project:CreateBinary("test_1")
    :AddDependencies(project:CreateDependency()
                         :AddSourceFiles("test_1.cpp")
                         :AddFlags("-Wall", "-Werror", "-Wextra")
                         :AddIncludeDirectories("../.."))

project:CreateBinary("test_2"):AddDependencies(
    project:CreateDependency()
        :AddSourceFiles("test_2.cpp")
        :AddFlags("-Wall", "-Werror", "-Wextra")
        :AddIncludeDirectories("../.."))

dep_perf = project:CreateDependency()
    :AddSourceFiles("test_perf.cpp")
    :AddFlags("-Wall", "-Werror", "-Wextra")
    :AddIncludeDirectories("../..")
project:CreateBinary("test_perf")
    :AddDependencies(dep_perf)

return project
