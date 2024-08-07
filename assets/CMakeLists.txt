# Make sure the user is not executing this script directly
if(NOT InMV)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InMV)

set(AssetsFiles
	"fonts/anewhope.fontdat"
	"fonts/anewhope_sharp1.fontdat"
	"fonts/anewhope_sharp1.tga"
	"fonts/anewhope_sharp2.fontdat"
	"fonts/anewhope_sharp2.tga"
	"fonts/anewhope_sharp3.fontdat"
	"fonts/anewhope_sharp3.tga"
	"fonts/anewhope_sharp4.fontdat"
	"fonts/anewhope_sharp4.tga"
	"fonts/anewhope_sharp5.fontdat"
	"fonts/anewhope_sharp5.tga"
	"fonts/anewhope.tga"
	"fonts/ergoec_sharp1.fontdat"
	"fonts/ergoec_sharp1.tga"
	"fonts/ergoec_sharp2.fontdat"
	"fonts/ergoec_sharp2.tga"
	"fonts/ergoec_sharp3.fontdat"
	"fonts/ergoec_sharp3.tga"
	"fonts/ergoec_sharp4.fontdat"
	"fonts/ergoec_sharp4.tga"
	"fonts/ergoec_sharp5.fontdat"
	"fonts/ergoec_sharp5.tga"
	"fonts/ocr_a.fontdat"
	"fonts/ocr_a_sharp1.fontdat"
	"fonts/ocr_a_sharp1.tga"
	"fonts/ocr_a_sharp2.fontdat"
	"fonts/ocr_a_sharp2.tga"
	"fonts/ocr_a_sharp3.fontdat"
	"fonts/ocr_a_sharp3.tga"
	"fonts/ocr_a_sharp4.fontdat"
	"fonts/ocr_a_sharp4.tga"
	"fonts/ocr_a.tga"

	"gfx/2d/code_new_roman_mip1.tga"
	"gfx/2d/code_new_roman_mip2.tga"
	"gfx/2d/code_new_roman_mip3.tga"
	"gfx/2d/code_new_roman.tga"
	"gfx/2d/demorec.tga"
	"gfx/2d/mplus_1m_bold_mip1.tga"
	"gfx/2d/mplus_1m_bold_mip2.tga"
	"gfx/2d/mplus_1m_bold_mip3.tga"
	"gfx/2d/mplus_1m_bold.tga"
	"gfx/menus/srv_102.jpg"
	"gfx/menus/srv_103.jpg"
	"gfx/menus/srv_104.jpg"

	"shaders/jk2mv.shader_mv"
	"shaders/sabers.dynGlow"

	"strip/mv.sp"

	"ui/jk2mpingame.txt"
	"ui/jk2mp/controls.menu_patch"
	"ui/jk2mp/createserver.menu_patch"
	"ui/jk2mp/ingame_controls.menu_patch"
	"ui/jk2mp/ingame_player.menu_patch"
	"ui/jk2mp/ingame_setup.menu_patch"
	"ui/jk2mp/ingame_setup_modoptions.menu"
	"ui/jk2mp/joinserver.menu_patch"
	"ui/jk2mp/setup.menu_patch"
	"ui/jk2mv/download_info.menu"
	"ui/jk2mv/download_popup.menu"

	"vm/cgame_102.map"
	"vm/cgame_103.map"
	"vm/cgame_104.map"
	"vm/jk2mpgame_102.map"
	"vm/jk2mpgame_103.map"
	"vm/jk2mpgame_104.map"
	"vm/ui_102.map"
	"vm/ui_103.map"
	"vm/ui_104.map"
)

source_group("assetsmv" FILES ${AssetsFiles})

# generate assetsmv.pk3
if(MultiConfigGenerator)
	set(OUTFILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/base/assetsmv.pk3)
else()
	set(OUTFILE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/base/assetsmv.pk3)
endif()

include(InstallZIP)
add_zip_command(${CMAKE_CURRENT_BINARY_DIR}/assetsmv.pk3 ${OUTFILE}
	DIR "${CMAKE_SOURCE_DIR}/assets"
	FILES "${AssetsFiles}"
	DEPENDS "${AssetsFiles}"
)

add_custom_target(assetsmv DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/assetsmv.pk3" ${AssetsFiles})

if(WIN32 OR BuildPortableVersion)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/assetsmv.pk3 DESTINATION "base")
elseif(APPLE)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/assetsmv.pk3 DESTINATION "jk2mvmp.app/Contents/MacOS/base")
else()
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/assetsmv.pk3 DESTINATION "share/jk2mv/base")
endif()
