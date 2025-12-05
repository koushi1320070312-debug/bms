import React, { useState, useEffect } from 'react';
import { initializeApp } from 'firebase/app';
import { getAuth, signInAnonymously, signInWithCustomToken, onAuthStateChanged } from 'firebase/auth';
import { getFirestore, doc, getDoc, setDoc, collection, query, where, getDocs } from 'firebase/firestore';

// グローバル変数（Canvas環境から提供される）の宣言
// 実際には環境によって提供されますが、ESLintなどの警告を避けるために一応定義
const appId = typeof __app_id !== 'undefined' ? __app_id : 'default-app-id';
const firebaseConfig = typeof __firebase_config !== 'undefined' ? JSON.parse(__firebase_config) : {};
const initialAuthToken = typeof __initial_auth_token !== 'undefined' ? __initial_auth_token : null;

// Firebaseサービスの初期化
const app = initializeApp(firebaseConfig);
const db = getFirestore(app);
const auth = getAuth(app);


// =================================================================
// 1. データ構造と初期値
// =================================================================

// デフォルト設定
const DEFAULT_CONFIG = {
    scrollSpeed: 3.5, // ハイスピード設定 (3.5x)
    gaugeMode: 'NORMAL', // ゲージの種類 (NORMAL, HARD, EASY)
    judgeOffset: 0, // 判定オフセット (ms)
};

const MOCK_MUSIC_ID = "MOCK_BMS_001"; // モック用楽曲ID

// デフォルトの7キーバインド (キーコード: JavaScriptの key.code または key.keyCode を想定)
// ここでは一般的なPCキーボードのキーコードを想定
const DEFAULT_KEYBINDS = {
    keyToLane: {
        90: 11, // Z -> レーン1 (白)
        83: 12, // S -> レーン2 (青)
        88: 13, // X -> レーン3 (白)
        68: 14, // D -> レーン4 (青)
        67: 15, // C -> レーン5 (白)
        70: 16, // F -> レーン6 (青)
        86: 17, // V -> レーン7 (白)
    },
    laneToKeyName: {
        11: 'Z',
        12: 'S',
        13: 'X',
        14: 'D',
        15: 'C',
        16: 'F',
        17: 'V',
    }
};

const LANE_NAMES = {
    11: "1P SCRATCH", // 実際は1P白鍵1
    12: "1P KEY 1",   // 実際は1P黒鍵1
    13: "1P KEY 2",
    14: "1P KEY 3",
    15: "1P KEY 4",
    16: "1P KEY 5",
    17: "1P KEY 6",
    18: "1P KEY 7"
};

// =================================================================
// 2. メインコンポーネント
// =================================================================

const App = () => {
    // 状態管理
    const [config, setConfig] = useState(DEFAULT_CONFIG);
    const [highScores, setHighScores] = useState([]);
    const [status, setStatus] = useState("初期化中...");
    const [isLoading, setIsLoading] = useState(false);
    const [isAuthReady, setIsAuthReady] = useState(false);
    const [userId, setUserId] = useState(null);
    const [keybinds, setKeybinds] = useState(DEFAULT_KEYBINDS);
    const [isBinding, setIsBinding] = useState(null); // どのレーンをバインド中か (例: 11)
    
    // --------------------------------------------------------
    // A. Firebase 初期化と認証
    // --------------------------------------------------------
    useEffect(() => {
        const setupAuth = async () => {
            try {
                if (initialAuthToken) {
                    await signInWithCustomToken(auth, initialAuthToken);
                } else {
                    await signInAnonymously(auth);
                }
            } catch (error) {
                console.error("Firebase Auth Error:", error);
                setStatus("認証エラーが発生しました。");
            }
        };

        const unsubscribe = onAuthStateChanged(auth, (user) => {
            if (user) {
                setUserId(user.uid);
                setIsAuthReady(true);
                console.log("Authenticated User ID:", user.uid);
            } else {
                setUserId(null);
                setIsAuthReady(true); // 匿名サインインが失敗した場合も一応Readyにする
                setStatus("匿名でサインインできませんでした。");
            }
        });

        setupAuth();
        return () => unsubscribe();
    }, []);

    // --------------------------------------------------------
    // C. スコアデータのロード
    // --------------------------------------------------------
    useEffect(() => {
        if (!isAuthReady || !db) return;

        // 公開されているハイスコアのコレクションパス
        const highscoreCollectionPath = `artifacts/${appId}/public/data/highScores`;
        
        const loadHighScores = async () => {
            try {
                // スコアのロード (ソートはJS側で行うため、クエリでは行わない)
                const q = query(collection(db, highscoreCollectionPath), where("musicId", "==", MOCK_MUSIC_ID));
                const querySnapshot = await getDocs(q);

                const scores = [];
                querySnapshot.forEach((doc) => {
                    scores.push(doc.data());
                });

                // スコアを降順（高得点順）でソート
                scores.sort((a, b) => b.score - a.score);
                setHighScores(scores.slice(0, 10)); // Top 10のみ保持

            } catch (error) {
                console.error("High Score Load Error:", error);
                setStatus("ハイスコアのロード中にエラーが発生しました。");
            }
        };

        loadHighScores();
    }, [isAuthReady]);


    // --------------------------------------------------------
    // B. 設定データのロードと保存（キーバインドの統合）
    // --------------------------------------------------------

    const userSettingsPath = `artifacts/${appId}/users/${userId}/settings/gameConfig`;

    // 起動時に設定とキーバインドをロード
    useEffect(() => {
        if (!isAuthReady || !userId || !db) return;

        const loadConfig = async () => {
            setIsLoading(true);
            try {
                const docRef = doc(db, userSettingsPath);
                const docSnap = await getDoc(docRef);

                if (docSnap.exists()) {
                    const data = docSnap.data();
                    setConfig(data);
                    // キーバインドもロード
                    if (data.keybinds) {
                        setKeybinds(data.keybinds);
                    }
                    setStatus("設定とキーバインドをロードしました。");
                } else {
                    await saveConfig({...DEFAULT_CONFIG, keybinds: DEFAULT_KEYBINDS}, false); // 初回保存
                    setStatus("初期設定を作成・保存しました。");
                }
            } catch (error) {
                console.error("設定ロードエラー:", error);
                setStatus("設定のロード中にエラーが発生しました。");
            } finally {
                setIsLoading(false);
            }
        };

        loadConfig();
    }, [isAuthReady, userId]);

    // 設定の保存関数（キーバインドを統合）
    const saveConfig = async (newConfig, showStatus = true) => {
        if (!isAuthReady || !userId || !db) return;
        setIsLoading(true);
        try {
            const dataToSave = {...newConfig, keybinds: keybinds}; // キーバインドを統合
            await setDoc(doc(db, userSettingsPath), dataToSave);
            setConfig(newConfig);
            if (showStatus) setStatus("設定とキーバインドをクラウドに保存しました。");
        } catch (error) {
            console.error("設定保存エラー:", error);
            setStatus("設定の保存中にエラーが発生しました。");
        } finally {
            setIsLoading(false);
        }
    };

    // --------------------------------------------------------
    // D. モックのハイスコア保存 (デモ用)
    // --------------------------------------------------------
    const saveMockScore = async () => {
        if (!isAuthReady || !userId || !db) {
            setStatus("認証が完了していません。");
            return;
        }

        const score = Math.floor(Math.random() * 50000) + 50000; // 50000-100000
        
        const scoreData = {
            musicId: MOCK_MUSIC_ID,
            score: score,
            maxCombo: 500,
            judge: { pgreat: 200, great: 50, good: 10, bad: 5, poor: 2 },
            timestamp: new Date().toISOString(),
            userId: userId, 
            userName: `User_${userId.substring(0, 4)}`,
            scrollSpeed: config.scrollSpeed,
        };

        try {
            const docRef = doc(db, `artifacts/${appId}/public/data/highScores`, `${userId}_${MOCK_MUSIC_ID}`);
            await setDoc(docRef, scoreData, { merge: true });
            setStatus(`新しいハイスコア ${score} をクラウドに保存しました！`);
        } catch (error) {
            console.error("Score Save Error:", error);
            setStatus("スコアの保存中にエラーが発生しました。");
        }
    };

    // --------------------------------------------------------
    // E. キーバインド操作ロジック
    // --------------------------------------------------------
    
    // キーバインド開始
    const startBinding = (lane) => {
        if (isLoading) return;
        setIsBinding(lane);
        setStatus(`「${LANE_NAMES[lane]}」に割り当てるキーを押してください...`);
    };

    // キーバインド実行（キーイベントリスナー）
    useEffect(() => {
        if (isBinding === null) return;

        const handleKeyDown = (e) => {
            e.preventDefault();
            e.stopPropagation();

            // key.codeではなく、互換性の高いe.keyCodeを使用
            const keyCode = e.keyCode;
            const newKeyToLane = { ...keybinds.keyToLane };
            const newLaneToKeyName = { ...keybinds.laneToKeyName };
            const newKeyName = e.key.toUpperCase() === ' ' ? 'SPACE' : e.key.toUpperCase();
            
            // 既存のマッピングを削除 (キーが重複しないように)
            // 他のレーンに割り当てられているキーコードを削除
            const existingMapping = Object.entries(newKeyToLane).find(([keyStr, lane]) => parseInt(keyStr) === keyCode);

            if (existingMapping) {
                const [existingKeyStr, existingLane] = existingMapping;
                // 古いキーコードの割り当てを削除
                delete newKeyToLane[existingKeyStr]; 
                // 古いレーン名も削除
                delete newLaneToKeyName[existingLane]; 
            }
            
            // 新しいマッピングを設定
            newKeyToLane[keyCode] = isBinding;
            newLaneToKeyName[isBinding] = newKeyName;

            setKeybinds({ keyToLane: newKeyToLane, laneToKeyName: newLaneToKeyName });
            setIsBinding(null); // バインド終了
            setStatus(`「${newKeyName}」を「${LANE_NAMES[isBinding]}」に割り当てました。`);
        };

        window.addEventListener('keydown', handleKeyDown);
        return () => window.removeEventListener('keydown', handleKeyDown);

    }, [isBinding, keybinds]);

    const handleSaveClick = () => saveConfig(config);


    // --------------------------------------------------------
    // F. UI 要素
    // --------------------------------------------------------

    return (
        <div className="p-4 sm:p-8 bg-gray-50 min-h-screen font-sans">
            <h1 className="text-3xl font-extrabold text-gray-900 mb-6 text-center">
                <span className="text-blue-600">ReBMS</span> ゲーム設定・スコア管理
            </h1>

            <div className="text-sm font-medium text-gray-500 mb-4 p-3 bg-white rounded-lg shadow-sm">
                <p>ユーザーID (Private Data Prefix): <span className="text-purple-600 break-all">{userId || "Loading..."}</span></p>
                <p>アプリID (Artifact Prefix): <span className="text-green-600 break-all">{appId}</span></p>
            </div>

            <div className="flex flex-col lg:flex-row gap-8">
                
                {/* 設定パネル */}
                <div className="lg:w-1/2 p-6 bg-white shadow-xl rounded-xl">
                    <h2 className="text-2xl font-bold text-gray-800 mb-4">ゲーム設定</h2>

                    {/* スクロール速度 (ハイスピード) */}
                    <div className="mb-4">
                        <label className="block text-gray-700 font-semibold mb-2">スクロール速度 (HS: {config.scrollSpeed.toFixed(1)}x)</label>
                        <input
                            type="range"
                            min="0.5"
                            max="10.0"
                            step="0.1"
                            value={config.scrollSpeed}
                            onChange={(e) => setConfig({ ...config, scrollSpeed: parseFloat(e.target.value) })}
                            className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer range-lg"
                        />
                    </div>

                    {/* ゲージモード */}
                    <div className="mb-4">
                        <label className="block text-gray-700 font-semibold mb-2">ゲージモード</label>
                        <select
                            value={config.gaugeMode}
                            onChange={(e) => setConfig({ ...config, gaugeMode: e.target.value })}
                            className="w-full px-3 py-2 border border-gray-300 bg-white rounded-lg shadow-sm focus:outline-none focus:ring-blue-500 focus:border-blue-500"
                        >
                            <option value="NORMAL">NORMAL (標準)</option>
                            <option value="EASY">EASY (回復大)</option>
                            <option value="HARD">HARD (回復小・失敗あり)</option>
                        </select>
                    </div>

                    {/* 判定オフセット */}
                    <div className="mb-4">
                        <label className="block text-gray-700 font-semibold mb-2">判定オフセット ({config.judgeOffset.toFixed(0)} ms)</label>
                        <input
                            type="range"
                            min="-100"
                            max="100"
                            step="1"
                            value={config.judgeOffset}
                            onChange={(e) => setConfig({ ...config, judgeOffset: parseInt(e.target.value) })}
                            className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer range-lg"
                        />
                        <p className="text-xs text-gray-500 mt-1">正の値: 早く判定 (音源を遅く聞く/映像を早く見る場合)</p>
                    </div>

                    {/* ★ キーバインド設定パネル (新規) */}
                    <h3 className="text-xl font-semibold text-gray-700 mt-6 mb-3 border-t pt-4">キーバインド設定 (7KEY)</h3>
                    <div className="space-y-2">
                        {[11, 12, 13, 14, 15, 16, 17].map(lane => (
                            <div key={lane} className="flex justify-between items-center p-3 bg-blue-50 rounded-lg">
                                <span className="font-medium text-gray-700">{LANE_NAMES[lane]}</span>
                                <button 
                                    onClick={() => startBinding(lane)}
                                    className={`px-4 py-2 rounded-lg font-bold transition ${
                                        isBinding === lane 
                                            ? 'bg-red-500 text-white animate-pulse' 
                                            : 'bg-blue-400 text-white hover:bg-blue-500'
                                    }`}
                                    disabled={isLoading}
                                >
                                    {isBinding === lane ? '押鍵中...' : (keybinds.laneToKeyName[lane] || '未設定')}
                                </button>
                            </div>
                        ))}
                    </div>

                    {/* 保存ボタンとステータス */}
                    <button 
                        onClick={handleSaveClick}
                        className="w-full mt-4 flex items-center justify-center px-4 py-3 bg-green-500 text-white text-lg font-bold rounded-xl hover:bg-green-600 transition duration-150 shadow-md"
                        disabled={isLoading || !isAuthReady}
                    >
                        <svg className="w-6 h-6 mr-2" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M8 7H5a2 2 0 00-2 2v9a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-3m-1 4l-3 3m0 0l-3-3m3 3V4"></path></svg>
                        {isLoading ? '保存中...' : '設定をクラウドに保存'}
                    </button>
                    
                    <p className={`mt-3 text-center text-sm ${status.includes('エラー') ? 'text-red-500' : 'text-gray-600'}`}>
                        {status}
                    </p>
                </div>

                {/* ハイスコアパネル */}
                <div className="lg:w-1/2 p-6 bg-white shadow-xl rounded-xl">
                    <h2 className="text-2xl font-bold text-gray-800 mb-4">モックハイスコア ({MOCK_MUSIC_ID})</h2>
                    
                    <button 
                        onClick={saveMockScore}
                        className="w-full flex items-center justify-center px-4 py-3 mb-4 bg-yellow-500 text-white text-lg font-bold rounded-xl hover:bg-yellow-600 transition duration-150 shadow-md"
                        disabled={!isAuthReady}
                    >
                        <svg className="w-6 h-6 mr-2" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg"><path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M9 12l2 2 4-4m5.618-4.108a12.02 12.02 0 00-15.018 0m15.018 0a12.02 12.02 0 010 15.018m-15.018 0a12.02 12.02 0 000-15.018"></path></svg>
                        仮のゲームプレイ結果を保存 (デモ)
                    </button>

                    <div className="overflow-x-auto">
                        <table className="min-w-full divide-y divide-gray-200">
                            <thead className="bg-gray-50">
                                <tr>
                                    <th className="px-3 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">#</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Score</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">User</th>
                                    <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">HS</th>
                                </tr>
                            </thead>
                            <tbody className="bg-white divide-y divide-gray-200">
                                {highScores.map((score, index) => (
                                    <tr key={index} className={score.userId === userId ? 'bg-yellow-50' : ''}>
                                        <td className="px-3 py-4 whitespace-nowrap text-sm font-medium text-gray-900">{index + 1}</td>
                                        <td className="px-6 py-4 whitespace-nowrap text-lg font-bold text-blue-600">{score.score}</td>
                                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-700">
                                            {score.userName} 
                                            {score.userId === userId && <span className="ml-2 text-xs text-purple-500">(YOU)</span>}
                                        </td>
                                        <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">{score.scrollSpeed.toFixed(1)}x</td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                        {highScores.length === 0 && <p className="p-4 text-center text-gray-500">ハイスコアはまだありません。</p>}
                    </div>

                </div>

            </div>
        </div>
    );
};

export default App;
