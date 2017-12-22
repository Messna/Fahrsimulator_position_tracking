using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public class CreateObject : MonoBehaviour {

    public GameObject TestObject;
    public Client KinectClient;

    // Use this for initialization
    void Start () {
    }
	
	// Update is called once per frame
	void Update () {
        Vector3 test;
		/* Matrix4x4 transformation = new Matrix4x4 (new Vector4 (0.0936351f,-0.00920669f,-0.00660017f,-1.84357f),
			new Vector4 (-0.00285623f,-0.0723655f,0.0604231f,-5.73963f),
			new Vector4 (-0.0109621f,-0.0597858f,-0.0721204f,11.1304f),
												new Vector4 (0f, 0f, 0f, 1f)); */
		if (KinectClient.pointsDictionary.TryGetValue ("P1", out test)) {
			//Vector4 vec4 = new Vector4 (test.x, test.y, test.z, 1);
			//vec4 = transformation.MultiplyVector(vec4);
			TestObject.transform.position = new Vector3(test.x, test.y, test.z);
			//Instantiate (TestObject, test, new Quaternion ());
		} 
    }
}
