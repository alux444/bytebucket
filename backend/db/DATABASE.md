FOLDERS

- id (PK)
- name
- parent_id (FK to FOLDERS.id)

FILES

- id (PK)
- name
- folder_id (FK to FOLDERS.id)
- created_at
- updated_at
- size
- content_type

TAGS

- id (PK)
- name (unique)

FILE_TAGS

- file_id (FK to FILES.id)
- tag_id (FK to TAGS.id)

FILE_METADATA

- file_id (FK to FILES.id)
- key
- value
