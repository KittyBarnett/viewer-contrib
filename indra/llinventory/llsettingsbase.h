/**
* @file llsettingsbase.h
* @author optional
* @brief A base class for asset based settings groups.
*
* $LicenseInfo:2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2017, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#ifndef LL_SETTINGS_BASE_H
#define LL_SETTINGS_BASE_H

#include <string>
#include <map>
#include <vector>
#include <boost/signals2.hpp>

#include "llsd.h"
#include "llsdutil.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "llquaternion.h"
#include "v4color.h"
#include "v3color.h"
#include "llunits.h"

#include "llinventorysettings.h"

#define PTR_NAMESPACE     std
#define SETTINGS_OVERRIDE override

class LLSettingsBase : 
    public PTR_NAMESPACE::enable_shared_from_this<LLSettingsBase>,
    private boost::noncopyable
{
    friend class LLEnvironment;
    friend class LLSettingsDay;

    friend std::ostream &operator <<(std::ostream& os, LLSettingsBase &settings);

public:
    typedef F64Seconds Seconds;
    typedef F64        BlendFactor;
    typedef F32        TrackPosition; // 32-bit as these are stored in LLSD as such
    static const TrackPosition INVALID_TRACKPOS;

    static const std::string SETTING_ID;
    static const std::string SETTING_NAME;
    static const std::string SETTING_HASH;
    static const std::string SETTING_TYPE;

    typedef std::map<std::string, S32>  parammapping_t;

    typedef PTR_NAMESPACE::shared_ptr<LLSettingsBase> ptr_t;

    virtual ~LLSettingsBase() { };

    //---------------------------------------------------------------------
    virtual std::string getSettingType() const = 0;

    virtual LLSettingsType::type_e getSettingTypeValue() const = 0;

    //---------------------------------------------------------------------
    // Settings status 
    inline bool hasSetting(const std::string &param) const { return mSettings.has(param); }
    inline bool isDirty() const { return mDirty; }
    inline void setDirtyFlag(bool dirty) { mDirty = dirty; }

    size_t getHash() const; // Hash will not include Name, ID or a previously stored Hash

    inline LLUUID getId() const
    {
        return getValue(SETTING_ID).asUUID();
    }

    inline std::string getName() const
    {
        return getValue(SETTING_NAME).asString();
    }

    inline void setName(std::string val)
    {
        setValue(SETTING_NAME, val);
    }

    inline void replaceSettings(LLSD settings)
    {
        mSettings = settings;
        mBlendedFactor = 0.0;
        setDirtyFlag(true);
    }

    virtual LLSD getSettings() const;

    //---------------------------------------------------------------------
    // 
    inline void setLLSD(const std::string &name, const LLSD &value)
    {
        mSettings[name] = value;
        mDirty = true;
    }

    inline void setValue(const std::string &name, const LLSD &value)
    {
        setLLSD(name, value);
    }

    inline LLSD getValue(const std::string &name, const LLSD &deflt = LLSD()) const
    {
        if (!mSettings.has(name))
            return deflt;
        return mSettings[name];
    }

    inline void setValue(const std::string &name, F32 v)
    {
        setLLSD(name, LLSD::Real(v));
    }

    inline void setValue(const std::string &name, const LLVector2 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLVector3 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLVector4 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLQuaternion &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLColor3 &value)
    {
        setValue(name, value.getValue());
    }

    inline void setValue(const std::string &name, const LLColor4 &value)
    {
        setValue(name, value.getValue());
    }

    inline BlendFactor getBlendFactor() const
    {
        return mBlendedFactor;
    }

    // Note this method is marked const but may modify the settings object.
    // (note the internal const cast).  This is so that it may be called without
    // special consideration from getters.
    inline void     update() const
    {
        if (!mDirty)
            return;
        (const_cast<LLSettingsBase *>(this))->updateSettings();
    }

    virtual void    blend(const ptr_t &end, BlendFactor blendf) = 0;

    virtual bool    validate();

    virtual ptr_t   buildDerivedClone() = 0;

    class Validator
    {
    public:
        typedef boost::function<bool(LLSD &)> verify_pr;

        Validator(std::string name, bool required, LLSD::Type type, verify_pr verify = verify_pr(), LLSD defval = LLSD())  :
            mName(name),
            mRequired(required),
            mType(type),
            mVerify(verify),
            mDefault(defval)
        {   }

        std::string getName() const { return mName; }
        bool        isRequired() const { return mRequired; }
        LLSD::Type  getType() const { return mType; }

        bool        verify(LLSD &data);

        // Some basic verifications
        static bool verifyColor(LLSD &value);
        static bool verifyVector(LLSD &value, S32 length);
        static bool verifyVectorMinMax(LLSD &value, LLSD minvals, LLSD maxvals);
        static bool verifyVectorNormalized(LLSD &value, S32 length);
        static bool verifyQuaternion(LLSD &value);
        static bool verifyQuaternionNormal(LLSD &value);
        static bool verifyFloatRange(LLSD &value, LLSD range);
        static bool verifyIntegerRange(LLSD &value, LLSD range);

    private:
        std::string mName;
        bool        mRequired;
        LLSD::Type  mType;
        verify_pr   mVerify;
        LLSD        mDefault;
    };
    typedef std::vector<Validator> validation_list_t;

    static LLSD settingValidation(LLSD &settings, validation_list_t &validations);
protected:

    LLSettingsBase();
    LLSettingsBase(const LLSD setting);

    static LLSD settingValidation(LLSD settings);

    typedef std::set<std::string>   stringset_t;
    
    // combining settings objects. Customize for specific setting types
    virtual void lerpSettings(const LLSettingsBase &other, BlendFactor mix);
    LLSD    interpolateSDMap(const LLSD &settings, const LLSD &other, BlendFactor mix) const;

    /// when lerping between settings, some may require special handling.  
    /// Get a list of these key to be skipped by the default settings lerp.
    /// (handling should be performed in the override of lerpSettings.
    virtual stringset_t getSkipInterpolateKeys() const { return stringset_t(); }  

    // A list of settings that represent quaternions and should be slerped 
    // rather than lerped.
    virtual stringset_t getSlerpKeys() const { return stringset_t(); }

    // Calculate any custom settings that may need to be cached.
    virtual void updateSettings() { mDirty = false; };

    virtual validation_list_t getValidationList() const = 0;

    // Apply any settings that need special handling. 
    virtual void applySpecial(void *) { };

    virtual parammapping_t getParameterMap() const { return parammapping_t(); }

    LLSD        mSettings;
    bool        mIsValid;
    LLAssetID   mAssetID;

    LLSD        cloneSettings() const;

    inline void setBlendFactor(BlendFactor blendfactor) 
    {
        mBlendedFactor = blendfactor;
    }

    void markDirty() { mDirty = true; }

private:
    bool        mDirty;

    LLSD        combineSDMaps(const LLSD &first, const LLSD &other) const;

    BlendFactor mBlendedFactor;
};


class LLSettingsBlender : public PTR_NAMESPACE::enable_shared_from_this<LLSettingsBlender>
{
public:
    typedef PTR_NAMESPACE::shared_ptr<LLSettingsBlender>      ptr_t;
    typedef boost::signals2::signal<void(const ptr_t )> finish_signal_t;
    typedef boost::signals2::connection     connection_t;

    LLSettingsBlender(const LLSettingsBase::ptr_t &target,
            const LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting) :
        mOnFinished(),
        mTarget(target),
        mInitial(initsetting),
        mFinal(endsetting)
    {
        if (mInitial)
            mTarget->replaceSettings(mInitial->getSettings());

        if (!mFinal)
            mFinal = mInitial;
    }

    virtual ~LLSettingsBlender() {}

    virtual void reset( LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting, const LLSettingsBase::TrackPosition&)
    {
        // note: the 'span' reset parameter is unused by the base class.
        if (!mInitial)
            LL_WARNS("BLENDER") << "Reseting blender with empty initial setting. Expect badness in the future." << LL_ENDL;

        mInitial = initsetting;
        mFinal = endsetting;

        if (!mFinal)
            mFinal = mInitial;

        mTarget->replaceSettings(mInitial->getSettings());
    }

    LLSettingsBase::ptr_t getTarget() const
    {
        return mTarget;
    }

    LLSettingsBase::ptr_t getInitial() const
    {
        return mInitial;
    }

    LLSettingsBase::ptr_t getFinal() const
    {
        return mFinal;
    }

    connection_t            setOnFinished(const finish_signal_t::slot_type &onfinished)
    {
        return mOnFinished.connect(onfinished);
    }

    virtual void            update(const LLSettingsBase::BlendFactor& blendf);
    virtual void            applyTimeDelta(const LLSettingsBase::Seconds& delta)
    {
        llassert(false);
        // your derived class needs to implement an override of this func
    }

    virtual F64             setBlendFactor(const LLSettingsBase::BlendFactor& position);

    virtual void            switchTrack(S32 trackno, const LLSettingsBase::TrackPosition& position) { /*NoOp*/ }

protected:
    void                    triggerComplete();

    finish_signal_t         mOnFinished;

    LLSettingsBase::ptr_t   mTarget;
    LLSettingsBase::ptr_t   mInitial;
    LLSettingsBase::ptr_t   mFinal;
};

class LLSettingsBlenderTimeDelta : public LLSettingsBlender
{
public:
    LLSettingsBlenderTimeDelta(const LLSettingsBase::ptr_t &target,
        const LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting, const LLSettingsBase::Seconds& blend_span) :
        LLSettingsBlender(target, initsetting, endsetting),
        mBlendSpan(blend_span),
        mLastUpdate(0.0f),
        mTimeSpent(0.0f)
    {
        mTimeStart = LLSettingsBase::Seconds(LLDate::now().secondsSinceEpoch());
        mLastUpdate = mTimeStart;
    }

    virtual ~LLSettingsBlenderTimeDelta() 
    {
    }

    virtual void reset(LLSettingsBase::ptr_t &initsetting, const LLSettingsBase::ptr_t &endsetting, const LLSettingsBase::TrackPosition& blend_span) SETTINGS_OVERRIDE
    {
        LLSettingsBlender::reset(initsetting, endsetting, blend_span);

        mBlendSpan  = blend_span;
        mTimeStart  = LLSettingsBase::Seconds(LLDate::now().secondsSinceEpoch());
        mLastUpdate = mTimeStart;
        mTimeSpent  = LLSettingsBase::Seconds(0.0);
    }

    virtual void applyTimeDelta(const LLSettingsBase::Seconds& timedelta) SETTINGS_OVERRIDE;

protected:
    LLSettingsBase::BlendFactor calculateBlend(const LLSettingsBase::TrackPosition& spanpos, const LLSettingsBase::TrackPosition& spanlen) const;

    LLSettingsBase::TrackPosition mBlendSpan;
    LLSettingsBase::Seconds mLastUpdate;
    LLSettingsBase::Seconds mTimeSpent;
    LLSettingsBase::Seconds mTimeStart;
};


#endif
