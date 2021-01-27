glsLangValidator.exe -V raygen.rgen -o raygen.spv --target-env vulkan1.2
glsLangValidator.exe -V miss.rmiss -o miss.spv --target-env vulkan1.2
glsLangValidator.exe -V shadow.rmiss -o shadow.spv --target-env vulkan1.2
glsLangValidator.exe -V closesthit.rchit -o closesthit.spv --target-env vulkan1.2
glsLangValidator.exe -V intersection.rint -o intersection.spv --target-env vulkan1.2
glsLangValidator.exe -V closesthitsphere.rchit -o closesthitsphere.spv --target-env vulkan1.2
pause