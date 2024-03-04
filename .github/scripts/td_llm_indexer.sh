#!/bin/bash

# Download requirements
set -euxo pipefail
cd llm-target-determinator
pip install -r requirements.txt
cd ../codellama
pip install -e .

# Download checkpoint
pip install awscli==1.32.18
cd codellama/
mkdir "CodeLlama-7b-Python"
aws s3 cp \
"s3://target-determinator-assets/CodeLlama-7b-Python" \
"CodeLlama-7b-Python" \
--recursive

# Run indexer
cd "${GITHUB_WORKSPACE}"/llm-target-determinator

python create_filelist.py

torchrun \
    --standalone \
    --nnodes=1 \
    --nproc-per-node=1 \
    indexer.py \
    --experiment-name indexer-files

# Upload the index to S3
cd "${GITHUB_WORKSPACE}"/llm-target-determinator/assets

TIMESTAMP=$(date -Iseconds)
ZIP_NAME = "indexer-files-${TIMESTAMP}.zip"

# Create a zipfile with all the generated indices
zip -r "${ZIP_NAME}" indexer-files

# Move the old index into the archived/ folder
aws s3 cp \
"s3://target-determinator-assets/indexes/latest/*" \
"s3://target-determinator-assets/indexes/archived/"

# Move the new index into the latestl/ folder
aws s3 cp \
"${ZIP_NAME}" \
"s3://target-determinator-assets/indexes/latest/${ZIP_NAME}"

# Note that because the above 2 operations are not atomic, there will
# be a period of a few seconds between these where there is no index
# present in the latest/ folder. To account for this, the retriever
# should have some retry logic with backoff to ensure fetching the
# index doesn't fail.
