# HelmBoy Documentation

This directory contains the Doxygen documentation system for HelmBoy Synthesizer.

## Overview

The documentation is generated using Doxygen and provides comprehensive coverage of:

- **API Reference**: Detailed documentation of all classes, methods, and functions
- **Architecture Guide**: High-level system design and component interactions
- **User Interface**: Documentation of GUI components and user interaction
- **Plugin System**: Integration with DAWs and plugin host communication
- **Development Guide**: Coding standards, build instructions, and contribution guidelines

## Generating Documentation

### Prerequisites

- **Doxygen**: Install from [doxygen.nl](https://www.doxygen.nl/download.html)
- **Graphviz** (optional): For generating diagrams and call graphs

### Generation Scripts

Use the provided scripts to generate documentation:

**Windows (PowerShell):**

```powershell
.\generate_docs.ps1
```

**Linux/macOS (Bash):**

```bash
chmod +x generate_docs.sh
./generate_docs.sh
```

**Manual Generation:**

```bash
doxygen Doxyfile
```

### Output

Documentation is generated in the `docs/html/` directory. Open `docs/html/index.html` in your web browser to view the complete documentation.

## Documentation Structure

### Source Files

- **`mainpage.dox`**: Main documentation page with overview and getting started
- **`synthesis_group.dox`**: Synthesis engine architecture and components
- **`ui_group.dox`**: User interface system and components
- **`plugin_group.dox`**: Plugin architecture and host integration
- **`contributing.dox`**: Development guidelines and contribution procedures

### Generated Content

The documentation system automatically extracts information from:

- **Header files** (`.h`): Class and function declarations with Doxygen comments
- **Source files** (`.cpp`): Implementation details and additional documentation
- **README files**: Project overview and build instructions
- **Configuration**: Doxyfile settings for output customization

## Documentation Standards

### Doxygen Comments

All public APIs should include Doxygen documentation:

```cpp
/**
 * @brief Brief description of the function
 *
 * Detailed description explaining the purpose, behavior,
 * and any important considerations.
 *
 * @param input_param Description of input parameter
 * @param output_param Description of output parameter
 * @return Description of return value
 *
 * @see RelatedClass
 * @see related_function()
 *
 * @warning Important warnings about usage
 * @note Additional notes for developers
 */
ReturnType functionName(InputType input_param, OutputType& output_param);
```

### Documentation Groups

Use `@defgroup` and `@ingroup` to organize related functionality:

```cpp
/**
 * @defgroup synthesis_engine Synthesis Engine
 * @brief Core audio processing components
 */

/**
 * @brief Oscillator class
 * @ingroup synthesis_engine
 */
class Oscillator {
    // ...
};
```

### Cross-References

Link related components using `@see` references:

```cpp
/**
 * @brief Main synthesis engine
 * @see HelmBoyVoiceHandler for voice management
 * @see HelmPlugin for plugin integration
 */
class HelmBoyEngine {
    // ...
};
```

## Customization

### Doxyfile Configuration

Key settings in `Doxyfile`:

- **`PROJECT_NAME`**: "HelmBoy Synthesizer"
- **`OUTPUT_DIRECTORY`**: `docs`
- **`INPUT`**: Source directories and documentation files
- **`EXTRACT_ALL`**: Extract documentation from all files
- **`GENERATE_HTML`**: Generate HTML output
- **`GENERATE_LATEX`**: Disabled by default

### Styling

The documentation uses Doxygen's default styling with custom color scheme:

- **Primary**: Blue tones for headings and navigation
- **Accent**: Green highlights for code and examples
- **Background**: Light theme for readability

### Diagrams

Class diagrams and call graphs are generated automatically when Graphviz is available:

- **Class hierarchies**: Inheritance relationships
- **Collaboration diagrams**: Class interactions
- **Include graphs**: Header file dependencies

## Maintenance

### Regular Updates

- **API Changes**: Update documentation when modifying public interfaces
- **New Features**: Add documentation for new components and functionality
- **Examples**: Include code examples for complex APIs
- **Troubleshooting**: Document common issues and solutions

### Quality Checks

- **Completeness**: Ensure all public APIs are documented
- **Accuracy**: Verify documentation matches implementation
- **Clarity**: Write clear, concise descriptions
- **Links**: Check cross-references and external links

## Contributing

When contributing to the documentation:

1. **Follow Standards**: Use consistent Doxygen formatting
2. **Test Generation**: Verify documentation builds correctly
3. **Review Output**: Check generated HTML for formatting issues
4. **Update Index**: Add new pages to appropriate groups
5. **Spell Check**: Use proper spelling and grammar

## Support

For documentation-related issues:

- **Build Problems**: Check Doxygen installation and version
- **Missing Content**: Verify input files and paths in Doxyfile
- **Formatting Issues**: Review Doxygen comment syntax
- **Feature Requests**: Suggest improvements via GitHub issues

The documentation system is continuously improved to provide the best possible reference for HelmBoy development and usage
