include("C:/Users/thoma/OneDrive/Desktop/Automata/build/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Automata-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Users/thoma/OneDrive/Desktop/Automata/build/Automata.exe"
    GENERATE_QT_CONF
)
