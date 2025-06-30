rdma_shared_provider(cxi libcxi.map
	1 1.0.${PACKAGE_VERSION}
	cxi.c
	verbs.c
)

publish_headers(infiniband
	cxidv.h
)

install(FILES "cxi.driver" DESTINATION "${CONFIG_DIR}")

rdma_pkg_config("cxi" "libibverbs" "${CMAKE_THREAD_LIBS_INIT}")
