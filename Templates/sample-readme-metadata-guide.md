
# Requirements for samples browser

If you want your sample to be indexed by the Microsoft samples browser, you must include properly formatted metadata as the first item in the README.md file. If you omit the metadata, or if it's incorrectly formatted, the sample won't be discoverable through the browser.

The following shows a sample metadata block.

```yaml
---
page_type: sample
languages:
- c
products:
- azure
- azure-sphere
name:
- name for sample; default is the text of the first H1 line below
urlFragment: HelloWorld
extendedZipContent:
- path: <location in repo>
  target: <location in zip>
- path: <location of next file>
  target: <target for next file>
---
```

The `languages` field lists the languages used in the sample code. [See language slugs](https://review.docs.microsoft.com/en-us/new-hope/information-architecture/metadata/taxonomies?branch=master#dev-lang).

The `products` field lists the Microsoft products that the samples demonstrates or uses. List additional products on separate lines, thusly:
products:
- azure
- azure-sphere
- azure-iot-sdk
See [product slugs](https://review.docs.microsoft.com/en-us/new-hope/information-architecture/metadata/taxonomies?branch=master#product) for a complete list.

The `urlFragment` is optional. The full URL will always be https://docs.microsoft.com/{locale}/samples/{repo-organization}/{repo-id}/{urlFragment}. The fragment may not contain the following characters: ", %, <, >, \, /, ^, *, :, ?, {, }, |, _, space, or backquote.

The `extendedZipContents` field lists additional folders and files that should be part of the downloaded zip file, in addition to the contents of the current folder, i.e. the folder that contains the README.md file. List as many of these as you need for your sample. See the [HelloWorld README.md file](https://github.com/Azure/azure-sphere-samples/blob/master/Samples/HelloWorld/README.md) for an example.

See [Metadata fields](https://review.docs.microsoft.com/en-us/help/contribute/samples/process/onboarding?branch=master#supported-metadata-fields-for-readmemd) in the Contributor Guide for more information and a full list of metadata.
