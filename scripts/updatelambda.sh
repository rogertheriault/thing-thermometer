#!/bin/bash
# run from main repo dir, pass the directory name and the lambda function name
# e.g.
# ./scripts/updatelambda.sh skill MySkill-dev

cd $1
# TODO add -f param only if zip exists
zip -dc -r ../lambda-$1.zip *
cd ..
aws lambda update-function-code --function-name $2 --zip-file fileb://lambda-$1.zip 
# rm lambda-$1.zip # leave this commented to use the "freshen" option
