set(SRCS
    default.frag
    default.vert
    full-screen.vert
    post.frag
    post.vert
    raytrace.rchit
    raytrace.rgen
    raytrace.rmiss
    raytraceShadow.rmiss
    ui-box.frag
    ui.frag
    ui-slider.frag
    ui-texture.frag
    ui.vert)

build_shaders()
install_shaders()
