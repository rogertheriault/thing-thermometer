#!/bin/bash
# run from main repo dir, pass the directory name and the lambda function name
# e.g.
# ./scripts/updatelambda.sh skill MySkill-dev

cd $1
zip -dc -r -f ../lambda-code.zip *
cd ..
aws lambda update-function-code --function-name $2 --zip-file fileb://lambda-code.zip 
# rm lambda-code.zip # leave this commented to use the "freshen" option
