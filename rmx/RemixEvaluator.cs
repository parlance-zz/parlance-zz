using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RemixEvaluator : MonoBehaviour
{
    public enum EvalResult { Restart, Continue, Success };

    public virtual void PrepaintMap(RemixMap map) { }
    public virtual EvalResult EvaluateMap(RemixMap map) { return EvalResult.Success; }
    public virtual void PostprocessMap(RemixMap map) { }
}
