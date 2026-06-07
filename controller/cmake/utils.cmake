
function(print_memsize target_name)
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/utils/print_memsize.sh ${target_name}.elf 0x200000 0x42000)
endfunction()