# WSPR Beakon - Licensing Information

This document provides detailed information about the licensing structure of the WSPR Beakon project.

## Dual License Structure

The WSPR Beakon project uses a **dual licensing approach** to appropriately cover different components of the project:

### 📄 Software Components - MIT License

**Covered Files:**
- `wspr-beakon.ino` (main firmware)
- All software documentation and code examples
- Software configuration files

**License File:** [`LICENSE.txt`](LICENSE.txt)

**Key Permissions:**
- ✅ Commercial use
- ✅ Modification
- ✅ Distribution
- ✅ Private use
- ✅ Sublicensing

**Requirements:**
- Include original copyright notice
- Include license text in all copies

### 🔧 Hardware Components - CERN-OHL-S

**Covered Files:**
- All PCB designs and Gerber files (`pcb/` directory)
- Schematic files
- 3D mechanical designs (`3D/` directory)
- Hardware documentation and assembly instructions
- Bill of materials (BOM)

**License File:** [`LICENSE-HARDWARE.txt`](LICENSE-HARDWARE.txt)

**Key Features:**
- ✅ Use, study, modify, share hardware designs
- ✅ Commercial manufacturing allowed
- ⚠️ **Strongly reciprocal**: Derivative hardware must use compatible open license
- ✅ Patent protection for contributors and users

## Why This Licensing Structure?

### For Software (MIT)
- **Maximum Compatibility**: Works with most other open source projects
- **Commercial Friendly**: Allows integration into commercial products
- **Minimal Restrictions**: Only requires attribution
- **Industry Standard**: Widely accepted in embedded software development

### For Hardware (CERN-OHL-S)
- **Open Hardware Standard**: Specifically designed for hardware projects
- **Community Protection**: Ensures improvements benefit everyone
- **Patent Safety**: Includes patent protection clauses
- **Reciprocal Nature**: Prevents proprietary forks of hardware designs

## Compliance Guidelines

### For Software Use
1. **Include Copyright Notice**: Maintain original copyright in all copies
2. **Include License**: Provide copy of MIT license with distribution
3. **No Warranty Disclaimer**: Software provided "as is"

### For Hardware Use
1. **Share Modifications**: Modified hardware designs must be made available
2. **Use Compatible License**: Derivative hardware must use CERN-OHL-S or compatible
3. **Provide Complete Source**: Include all files needed to manufacture
4. **Notice Requirements**: Mark products as licensed under CERN-OHL-S

## Licensing FAQs

### Can I sell products based on this design?
**Yes**, both licenses allow commercial use. However:
- Software modifications don't need to be shared back
- Hardware modifications **must** be shared under CERN-OHL-S

### Can I create closed-source derivatives?
- **Software**: Yes, under MIT license
- **Hardware**: No, CERN-OHL-S requires derivative hardware to remain open

### What if I only modify the software?
If you only modify firmware and use the original hardware design unchanged, you only need to comply with the MIT license for your software modifications.

### What if I only modify the hardware?
If you modify PCB designs or create derivative hardware, you must:
1. Share your hardware modifications under CERN-OHL-S
2. Provide complete source files for manufacturing
3. Can use any license for accompanying software

## Getting Help

For licensing questions:
- **Software licensing**: Contact EA1REX regarding MIT license compliance
- **Hardware licensing**: Contact EB1A regarding CERN-OHL-S compliance
- **General questions**: Open an issue in the project repository

## Additional Resources

- [MIT License Official Text](https://opensource.org/licenses/MIT)
- [CERN-OHL-S Official Documentation](https://cern-ohl.web.cern.ch/)
- [Open Source Hardware Association (OSHWA)](https://www.oshwa.org/)
- [CERN Knowledge and Technology Transfer](https://kt.cern/open-hardware-licence)

---

*This document is for informational purposes. The actual license texts in [`LICENSE.txt`](LICENSE.txt) and [`LICENSE-HARDWARE.txt`](LICENSE-HARDWARE.txt) are the authoritative legal documents.*