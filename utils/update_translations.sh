#!/bin/bash

# Function to update translations
update_translations() {
    source_folder="$1"
    pot_folder="$2"
    po_folder="$3"
    domain="$4"
    locales=("${@:5}")

    # Recursively find source files
    source_files=$(find "${source_folder}" -type f -name "*.cpp" -or -name "*.hpp")

    # Update or create the POT file
    pot_file="${pot_folder}/${domain}.pot"
    echo "Updating .pot file from sources..."
    xgettext --from-code=UTF-8 -c++ --keyword=_ --output="${pot_file}" ${source_files}

    # Update existing or create new PO files for each locale
    for locale in "${locales[@]}"; do
        po_file="${po_folder}/${locale}.po"
        if [[ -e "${po_file}" ]]; then
            echo "Updating ${locale} .po file.."
            msgmerge -U "${po_file}" "${pot_file}"
        else
            echo "Creating ${locale} .po file.."
            msginit --input="${pot_file}" --locale="${locale}" --output-file="${po_file}"
        fi
    done

    echo "Translation files updated."
}

# Check if required arguments are provided
if [ "$#" -lt 5 ]; then
    echo "Usage: $0 <source_folder> <pot_folder> <po_folder> <domain> <locales...>"
    exit 1
fi

# Run the update_translations function with command-line arguments
update_translations "$@"
