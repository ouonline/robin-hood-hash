project = Project()

flags = {"-Wall", "-Werror", "-Wextra"}

project:CreateBinary("test_1"):AddDependencies(
    project:CreateDependency()
        :AddSourceFiles("test_1.cpp")
        :AddFlags(flags)
        :AddIncludeDirectories("../.."))

project:CreateBinary("test_2"):AddDependencies(
    project:CreateDependency()
        :AddSourceFiles("test_2.cpp")
        :AddFlags(flags)
        :AddIncludeDirectories("../.."))

project:CreateBinary("test_perf"):AddDependencies(
    project:CreateDependency()
        :AddSourceFiles("test_perf.cpp")
        :AddFlags(flags)
        :AddIncludeDirectories("../.."))

return project
