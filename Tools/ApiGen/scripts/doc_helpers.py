"""
Helper functions for documentation processing.
"""
import os
import logging

MARKDOWN_EXT = '.md'
DOXYGEN_EXT = '.xml'

LOGGER = logging.getLogger("")

def process_folder(input_dir, output_dir, filter_ext, callback):
    """
    Process all files in a folder and its subfolders.
    ARGS:
        callback - callable(input_dir: str, output_dir: str, abs_file: str) : bool
    """
    result = True
    for root, dirs, files in os.walk(input_dir):
        for filename in files:
            if os.path.splitext(filename)[-1].endswith(filter_ext):
                abs_file = os.path.join(root, filename)
                LOGGER.info("Processing %s", os.path.relpath(abs_file, input_dir))
                result = callback(input_dir, output_dir, abs_file) and result
    return result

def update_file(file_path, contents):
    """
    Update or create output file.
    """
    dest_dir = os.path.dirname(file_path)
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)
    write = True
    if os.path.exists(file_path):
        with open(file_path, mode='r', encoding='utf-8') as handle:
            old_contents = handle.read()
        write = old_contents != contents
    if write:
        LOGGER.info("Writing: %s", file_path)
        with open(file_path, mode='w', encoding='utf-8') as handle:
            handle.write(contents)
    else:
        LOGGER.info("File up-to-date: %s", file_path)        
