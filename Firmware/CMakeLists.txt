cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(rp2040_audio_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# needed for Gigadevices flash chips
#set(PICO_DEFAULT_BOOT_STAGE2 boot2_generic_03h)

# Needed to keep multicore from choking the XIP cache
#set(PICO_COPY_TO_RAM 1)

# Initialize the SDK
pico_sdk_init()

# source files
add_executable(rp2040_audio
	main.c
	wm8731.c
	audio.c
	st7735.c
	adc.c
	i2s_fulldup.c
	debounce.c
	button.c
	gfx.c
	widgets.c
	menu.c
	dsp_lib.c
	fx.c
	fx_vca.c
	fx_cdl.c
	fx_rdly.c
	sinc_interp.c
	circbuf.c
	fx_idly.c
	fx_fsh.c
	ihilbert.c
	morph.c
	fx_phsr.c
	fx_pshf.c
	pshft.c
	fx_mv1.c
	mv_progs.c
	fx_rvb.c
	reverb.c
	fx_filters.c
	ifilter_mg4_v1.c
	nvs.c
)

pico_enable_stdio_uart(rp2040_audio 1)

pico_generate_pio_header(rp2040_audio ${CMAKE_CURRENT_LIST_DIR}/i2s_fulldup.pio)

target_link_libraries(rp2040_audio
	pico_stdlib
	hardware_pio
	hardware_dma
	hardware_i2c
	hardware_adc
	hardware_spi
	hardware_sync
	cmsis_core
	pico_multicore
    pico_unique_id
	pico_malloc
)

pico_add_extra_outputs(rp2040_audio)
