
NFSps.dll: dlldata.obj NFS_p.obj NFS_i.obj
	link /dll /out:NFSps.dll /def:NFSps.def /entry:DllMain dlldata.obj NFS_p.obj NFS_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del NFSps.dll
	@del NFSps.lib
	@del NFSps.exp
	@del dlldata.obj
	@del NFS_p.obj
	@del NFS_i.obj
