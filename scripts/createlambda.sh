#!/bin/bash
# run from main repo dir, pass the directory name and the lambda function name
# e.g.
# ./scripts/createlambda.sh skill MySkill-dev

# TODO replace with https://docs.aws.amazon.com/lambda/latest/dg/serverless-deploy-wt.html
if [ -z ${AWS_ACCOUNT_ID+x} ]; then
	echo "Please export AWS_ACCOUNT_ID first"
	exit
fi

ROLE_ARN="arn:aws:iam::${AWS_ACCOUNT_ID}:role/alexaLambdaThermometerRole"


cd $1
zip -r ../lambda-code.zip *
cd ..
aws lambda create-function \
	--function-name $2 \
	--runtime "nodejs6.10" \
	--role $ROLE_ARN \
	--handler "index.handler" \
	--description "Skill lambda handler for ESP8266 thermometer thing" \
	--timeout 7 \
	--zip-file fileb://lambda-code.zip 
rm lambda-code.zip
