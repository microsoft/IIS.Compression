Microsoft IIS Compression
--------------------------------

**IIS Compression** is a bundle of two IIS compression scheme providers `iisbrotli.dll` and `iiszlib.dll` that support IIS to compress HTTP response contents. `iisbrotli.dll` supports **Brotli** compression, while `iiszlib.dll` supports both **Gzip** and **Deflate** compression.

An IIS compression scheme provider is a pluggable extension of IIS `StaticCompressionModule` and `DynamicCompressionModule` - essentially a dynamic link library (dll) that implements and exposes the [IIS HTTP Compression API](https://msdn.microsoft.com/en-us/library/dd692872.aspx). IIS `StaticCompressionModule` and `DynamicCompressionModule` load the registered compression scheme providers into worker process at runtime, and leverage them to perform compression for static files and dynamically-generated contents, respectively.

### Installation
The IIS Compression installer registers `iisbrotli.dll` as the `br` (Brotli) compression scheme provider in `applicationHost.config`. It also replaces the default `gzip` compression scheme provider `gzip.dll` with `iiszlib.dll`. A sample `httpCompression` section in `applicationHost.config` is shown below:

```
<httpCompression directory="%SystemDrive%\inetpub\temp\IIS Temporary Compressed Files">
    <scheme name="br" dll="%windir%\system32\inetsrv\iisbrotli.dll" />
    <scheme name="gzip" dll="%windir%\system32\inetsrv\iiszlib.dll" />
    <staticTypes>
        ...
    </staticTypes>
    <dynamicTypes>
        ...
    </dynamicTypes>
</httpCompression>
```

### Build
1. Clone this project with submodules.
```
git clone --recurse-submodules https://github.com/Microsoft/IIS.Compression.git
```
2. Build the code using the `build.cmd` (the location of msbuild.exe needs to be added into `PATH` environment variable).

### Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
