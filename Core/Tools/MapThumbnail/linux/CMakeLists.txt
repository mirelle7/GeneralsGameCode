# freedesktop thumbnailer: GNOME Files, Nemo, Caja, Thunar (tumbler) and others run
# generals-map-thumbnailer for files of the MIME type the shared-mime-info entry declares.

install(FILES generals-map.thumbnailer DESTINATION share/thumbnailers)
install(FILES generals-map.xml DESTINATION share/mime/packages)
