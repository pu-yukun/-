using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;
using System.Linq; // 添加这行解决Distinct错误
public class NoobManager : MonoBehaviour
{
    [System.Serializable]
    public class TutorialStep
    {
        [TextArea(3, 10)]
        public string instruction;
        public List<KeyCode> enableKeys = new List<KeyCode>();
        [Tooltip("无需手动配置，系统自动处理")] 
        public UnityEvent onStepTrigger;
    }

    [Header("UI Components")]
    public Text instructionText;
    public GameObject tutorialPanel;

    [Header("Tutorial Settings")]
    public List<TutorialStep> tutorialSteps = new List<TutorialStep>();
    public List<KeyCode> allLockedKeys = new List<KeyCode>()
    {
        KeyCode.Mouse0, KeyCode.Mouse1, KeyCode.Mouse2,
        KeyCode.Space, KeyCode.G, KeyCode.F, KeyCode.Tab
    };

    private int currentStep = -1;
    private InputHelper inputHelper;

    void Start()
    {
        inputHelper = gameObject.AddComponent<InputHelper>();
        InitializeTutorial();
    }

    public void InitializeTutorial()
    {
        inputHelper.SetKeysEnabled(allLockedKeys, false);
        tutorialPanel.SetActive(true);
        MoveToNextStep();
    }

    public void MoveToNextStep()
    {
        currentStep++;
        
        if (currentStep >= tutorialSteps.Count)
        {
            CompleteTutorial();
            return;
        }

        UpdateUI();
        EnableCurrentStepKeys();
    }

    private void UpdateUI()
    {
        instructionText.text = tutorialSteps[currentStep].instruction;
    }

private void EnableCurrentStepKeys()
{
    // 前3步保持严格模式，后面允许复合操作
    currentInputMode = currentStep < 3 ? InputMode.TutorialLock : InputMode.FreeComposite;
    
    inputHelper.ClearAllCallbacks();
    
    // 设置按键状态（不再禁用已解锁的键）
    inputHelper.SetKeysEnabled(tutorialSteps[currentStep].enableKeys, true);
    
    if (currentInputMode == InputMode.TutorialLock)
    {
        // 严格模式：只监听当前步骤按键
        inputHelper.RegisterTemporaryCallbacks(
            tutorialSteps[currentStep].enableKeys,
            () => tutorialSteps[currentStep].onStepTrigger.Invoke()
        );
    }
    else
    {
        // 自由模式：监听所有已解锁键
        var allEnabledKeys = GetAllEnabledKeys();
        inputHelper.RegisterCompositeCallbacks(
            allEnabledKeys,
            (key) => {
                if(tutorialSteps[currentStep].enableKeys.Contains(key))
                    tutorialSteps[currentStep].onStepTrigger.Invoke();
            }
        );
    }
    
    tutorialSteps[currentStep].onStepTrigger.AddListener(MoveToNextStep);
}

private List<KeyCode> GetAllEnabledKeys()
{
    List<KeyCode> keys = new List<KeyCode>();
    for(int i = 0; i <= currentStep; i++)
    {
        keys.AddRange(tutorialSteps[i].enableKeys);
    }
    return keys.Distinct().ToList();
}


    private void CompleteTutorial()
    {
        inputHelper.SetKeysEnabled(allLockedKeys, true);
        tutorialPanel.SetActive(false);
        Debug.Log("新手教程完成！");
    }


public enum InputMode
{
    TutorialLock,    // 教程锁定模式（严格检测）
    FreeComposite    // 自由组合模式（允许复合操作）
}

private InputMode currentInputMode = InputMode.TutorialLock;

}

// InputHelper.cs (关键改进)
public class InputHelper : MonoBehaviour 
{
    public Dictionary<KeyCode, bool> keyEnabledStates = new Dictionary<KeyCode, bool>();
    private List<KeyCode> currentListeningKeys = new List<KeyCode>();
    private UnityAction currentCallback;

 private Dictionary<KeyCode, UnityAction<KeyCode>> compositeCallbacks = new Dictionary<KeyCode, UnityAction<KeyCode>>();

    public void RegisterCompositeCallbacks(List<KeyCode> keys, UnityAction<KeyCode> callback)
    {
        foreach(var key in keys)
        {
            compositeCallbacks[key] = callback;
        }
    }



    public void ClearAllCallbacks()
    {
        currentListeningKeys.Clear();
        currentCallback = null;
    }

    public void RegisterTemporaryCallbacks(List<KeyCode> keys, UnityAction callback)
    {
        currentListeningKeys = new List<KeyCode>(keys);
        currentCallback = callback;
    }



    void Update()
    {

  foreach(var kvp in compositeCallbacks)
        {
            if(Input.GetKeyDown(kvp.Key))
            {
                kvp.Value.Invoke(kvp.Key);
            }
        }




        // 优先处理禁用键
        foreach (var kvp in keyEnabledStates)
        {
            if (!kvp.Value && Input.GetKey(kvp.Key))
            {
                Input.ResetInputAxes();
                return; // 直接返回确保不被其他逻辑处理
            }
        }

        // 独占式按键检测
        foreach (var key in currentListeningKeys)
        {
            if (Input.GetKeyDown(key))
            {
                currentCallback?.Invoke();
                return; // 确保只响应一个按键
            }
        }
    }

    public void SetKeysEnabled(List<KeyCode> keys, bool enabled)
    {
        foreach (var key in keys)
        {
            keyEnabledStates[key] = enabled;
        }
    }
}
