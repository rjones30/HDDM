#!/usr/bin/env python3
#
# delete_pypi_project.py - script to automate removal of an existing Pypi project
#                          and deletion of all published wheels, needed to move
#                          an existing project to a new name or namespace.
#
# author: richard.t.jones at uconn.edu
# version: june 26, 2024
#
# note: This code was generated automatically by ChatGPT in response to a query.

import requests

# Replace with your API token and project name
api_token = 'your-api-token'
project_name = 'your-project-name'

headers = {
    'Authorization': f'Bearer {api_token}',
}

# Get the list of releases
url = f'https://pypi.org/pypi/{project_name}/json'
response = requests.get(url)
if response.status_code != 200:
    raise Exception(f"Failed to fetch project releases: {response.text}")

releases = response.json().get('releases', {}).keys()

# Delete each release
for version in releases:
    print(f"Deleting release {project_name}=={version}")
    url = f'https://pypi.org/manage/project/{project_name}/release/{version}/'
    response = requests.delete(url, headers=headers)
    if response.status_code != 204:
        print(f"Failed to delete release {version}: {response.text}")
    else:
        print(f"Successfully deleted release {version}")

# Delete the project
print(f"Deleting project {project_name}")
url = f'https://pypi.org/manage/project/{project_name}/'
response = requests.delete(url, headers=headers)
if response.status_code != 204:
    print(f"Failed to delete project: {response.text}")
else:
    print(f"Successfully deleted project {project_name}")
