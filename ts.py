import os
import xml.etree.ElementTree as ET

# Check if the file exists
dir = 'resources/i18n'
for filename in os.listdir(dir):
    file_path = os.path.join(dir, filename)
    if os.path.isfile(file_path):
        # Open the .ts file
        with open(file_path, "r", encoding="utf8") as file:
            country = filename[11:13]
            # Read the file content
            content = file.read()

            # Parse the XML content of the .ts file
            root = ET.fromstring(content)

            # Find all translation elements
            translations = root.findall(".//translation")

            # Count the completed translations
            translated = 0
            total = 0
            for translation in translations:
                if translation.text and translation.get('type') != 'vanished' and translation.get(
                        'type') != 'unfinished' and translation.get('type') != 'obsolete':
                    translated += 1
                if translation.get('type') != 'vanished' and translation.get('type') != 'obsolete':
                    total += 1
            perc = round(translated / total * 100)
            print(
                f'| <img src="https://flagcdn.com/48x36/{country.lower()}.png" width="24" height="18" alt="{country}"> {filename[8:13]} | {perc}% |')
    else:
        print("File not found.")
