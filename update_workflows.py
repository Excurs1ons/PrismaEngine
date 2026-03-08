import os
import glob
import re

workflow_dir = r"F:\repos\PrismaEngine\.github\workflows"
files = glob.glob(os.path.join(workflow_dir, "*.yml"))

cache_step_template = """

{indent}- name: Cache Dependencies
{indent}  uses: actions/cache@v4
{indent}  with:
{indent}    path: .dependencies
{indent}    key: ${{{{ runner.os }}-deps-${{{{ hashFiles('cmake/DependencyVersions.cmake') }}}} }}"""

for fpath in files:
    print(f"Processing {fpath}...")
    with open(fpath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Regex to find checkout step. 
    # It looks for '- name: ...' (optional) followed by 'uses: actions/checkout@v4'
    # and optionally a 'with:' block.
    # It also captures the indentation.
    
    # Matches:
    #   - name: Checkout
    #     uses: actions/checkout@v4
    #     with:
    #       submodules: recursive
    
    pattern = r"((?P<indent>\s+)-\s+(?:name:.*?\n\s+)?uses: actions/checkout@v4(?:\n(?P=indent)\s+with:.*?(?:\n(?P=indent)\s{3,}.*?)*)?)"
    
    def replacement(match):
        res = match.group(1)
        indent = match.group('indent')
        # Check if already cached to avoid duplicates
        if 'name: Cache Dependencies' in content:
             # This is a bit simplistic but works for now if we run it once
             # A better check would be to see if it's already after THIS checkout step
             pass
        
        return res + cache_step_template.format(indent=indent)

    new_content = re.sub(pattern, replacement, content)
    
    if new_content != content:
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated {fpath}")
    else:
        print(f"No changes for {fpath}")
