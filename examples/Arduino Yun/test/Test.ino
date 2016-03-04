#include <Parse.h>
#include <Bridge.h>
#include <assert.h>

/***** Integration Tests of Parse Arduino Yún SDK *****/
// NOTE: import Tempereture.json into yoru Parse app before running the test

int i = 0; // testId for a specific test

void basicObjectTest() {
  Serial.println("basic object operation test");

  Serial.println("create...");
  ParseObjectCreate create;
  create.setClassName("Temperature");
  create.add("temperature", 175.0);
  create.add("leverDown", true);
  ParseResponse createResponse = create.send();
  char* objectId = new char[10];
  strcpy(objectId, createResponse.getString("objectId"));
  assert(createResponse.getErrorCode() == 0);
  createResponse.close();

  Serial.println("update...");
  ParseObjectUpdate update;
  update.setClassName("Temperature");
  update.setObjectId(objectId);
  update.add("temperature", 100);
  ParseResponse updateResponse = update.send();
  assert(updateResponse.getErrorCode() == 0);
  updateResponse.close();

  Serial.println("get...");
  ParseObjectGet get;
  get.setClassName("Temperature");
  get.setObjectId(objectId);
  ParseResponse getResponse = get.send();
  double temp = getResponse.getDouble("temperature");
  assert(temp == 100);
  getResponse.close();

  Serial.println("delete...");
  ParseObjectDelete del;
  del.setClassName("Temperature");
  del.setObjectId(objectId);
  ParseResponse delResponse = del.send();
  String expectedResp = "{}";
  String actualResp = String(delResponse.getJSONBody());
  actualResp.trim();
  assert(expectedResp.equals(actualResp));
  delResponse.close();

  Serial.println("test passed\n");
}

void objectDataTypesTest() {
  Serial.println("data types test");

  ParseObjectCreate create;
  create.setClassName("TestObject");
  create.addGeoPoint("location", 40.0, -30.0);
  create.addJSONValue("dateField", "{\"__type\":\"Date\",\"iso\":\"2011-08-21T18:02:52.249Z\"}");
  create.addJSONValue("arrayField", "[30,\"s\"]");
  create.addJSONValue("emptyField", "null");
  ParseResponse createResponse = create.send();
  assert(createResponse.getErrorCode() == 0);
  createResponse.close();

  Serial.println("test passed\n");
}

void queryTest() {
  Serial.println("query test");

  ParseObjectCreate create1;
  create1.setClassName("Temperature");
  create1.add("temperature", 88.0);
  create1.add("leverDown", true);
  ParseResponse createResponse = create1.send();
  createResponse.close();

  ParseObjectCreate create2;
  create2.setClassName("Temperature");
  create2.add("temperature", 88.0);
  create2.add("leverDown", false);
  createResponse = create2.send();
  createResponse.close();

  ParseQuery query;
  query.setClassName("Temperature");
  query.whereEqualTo("temperature", 88);
  query.setLimit(2);
  query.setSkip(0);
  query.setKeys("temperature");
  ParseResponse response = query.send();

  int countOfResults = response.count();
  assert(countOfResults == 2);
  while(response.nextObject()) {
    assert(88 == response.getDouble("temperature"));
  }
  response.close();

  Serial.println("test passed\n");
}

void setup() {
  // Initialize digital pin 13 as an output.
  pinMode(13, OUTPUT);

  // Initialize Bridge
  Bridge.begin();

  // Initialize Serial
  Serial.begin(115200);

  while (!Serial); // wait for a serial connection

  // Initialize Parse
  Parse.begin("", "");
  Parse.setServerURL("https://my.server.somewhere/parse");
}

void loop() {
  if(i == 0) basicObjectTest();
  else if (i == 1) objectDataTypesTest();
  else if (i == 2) queryTest();
  else while(1);

  i++;
}
