# Modo v1.5.0 Release Notes

**Version:** 1.5.0  
**Date:** November 2026  
**Developer:** Atoma  

Modo v1.5.0 introduces full support for external downloadable Icon Sets from online catalogs and local archives, alongside rigorous security verification and seamless integration with Modo's adaptive styling engine.

---

### Key Features & Improvements

1. **Online & Local Icon Sets**:
   - Download and install curated artwork sets directly from JSON catalogs over HTTPS.
   - Install custom `.zip` icon packs directly from disk.
   - Artwork is matched by macOS bundle identifier (`bundleID`), substituting original app graphics while preserving full adaptive styling (Squircle, Pebble, Hexagon, Material You color harmonization, monochrome filters, and drop shadows).

2. **Defense-in-Depth Security**:
   - SHA-256 cryptographic verification of all downloaded sets.
   - Path-traversal containment prevents archives from writing outside their sandbox.
   - Strict rejection of symbolic links, zip bombs, oversized files (>6 MB per icon, >80 MB total), and corrupted image data.
   - Header inspection ensures every icon file is authentic PNG.

3. **Catalog Management & Custom Feeds**:
   - Built-in Icon Set Browser sheet accessible from the sidebar.
   - Customizable Catalog URL in Settings for private or community repositories.
   - Quick install, uninstall, and active set switching with instant cache invalidation.

4. **SelfTest & Verification**:
   - 38 automated end-to-end selftest checks verifying install pipelines, security boundaries, catalog retrieval, and rendering correctness.
