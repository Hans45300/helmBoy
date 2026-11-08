#!/bin/bash
# generate_docs.sh - Script to generate Doxygen documentation for HelmBoy

echo "Generating HelmBoy Synthesizer Documentation..."

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen is not installed. Please install Doxygen first."
    echo "On Windows: Download from https://www.doxygen.nl/download.html"
    echo "On macOS: brew install doxygen"
    echo "On Linux: sudo apt-get install doxygen (Ubuntu/Debian) or sudo yum install doxygen (CentOS/RHEL)"
    exit 1
fi

# Create docs directory if it doesn't exist
if [ ! -d "docs" ]; then
    mkdir -p docs
    echo "Created docs directory"
fi

# Update Doxyfile timestamp
echo "PROJECT_NUMBER = \"$(date +%Y.%m.%d)\"" >> Doxyfile.tmp
cat Doxyfile >> Doxyfile.tmp
mv Doxyfile.tmp Doxyfile

# Generate documentation
echo "Running Doxygen..."
doxygen Doxyfile

# Check if generation was successful
if [ $? -eq 0 ]; then
    echo "Documentation generated successfully!"
    echo "Open docs/html/index.html in your web browser to view the documentation."

    # Optional: Open documentation in default browser (uncomment if desired)
    # if command -v xdg-open &> /dev/null; then
    #     xdg-open docs/html/index.html
    # elif command -v open &> /dev/null; then
    #     open docs/html/index.html
    # elif command -v start &> /dev/null; then
    #     start docs/html/index.html
    # fi
else
    echo "Error: Documentation generation failed!"
    exit 1
fi

echo "Documentation generation complete."