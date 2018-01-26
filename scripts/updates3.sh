#!/bin/bash

# push the recipes.json to S3
aws s3 cp assets/recipes.json s3://thing-data/recipes.json \
	--acl public-read \
	--cache-control max-age=300
