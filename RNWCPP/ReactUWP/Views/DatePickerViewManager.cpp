// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include "DatePickerViewManager.h"
#include <Views/ShadowNodeBase.h>

#include <Utils/ValueUtils.h>

#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/Windows.Foundation.h>

namespace winrt {
  using namespace Windows::UI::Xaml;
  using namespace Windows::UI::Xaml::Controls;
  using namespace Windows::Globalization;
  using namespace Windows::Globalization::DateTimeFormatting;
  using namespace Windows::Foundation;
}

namespace react { namespace uwp {

class DatePickerShadowNode : public ShadowNodeBase
{
  using Super = ShadowNodeBase;
public:
  DatePickerShadowNode() = default;
  void createView() override;
  void updateProperties(const folly::dynamic&& props) override;

private:
  void OnDateChanged(
    IReactInstance& instance,
    int64_t tag,
    winrt::DateTime const& newDate);

  int64_t m_selectedTime, m_maxTime, m_minTime; // These values are expected to be in milliseconds
  int64_t m_timeZoneOffsetInSeconds = 0;        // Timezone offset is expected to be in seconds
};

void DatePickerShadowNode::createView()
{
  Super::createView();

  auto datePicker = GetView().as<winrt::CalendarDatePicker>();
  auto wkinstance = GetViewManager()->GetReactInstance();

  datePicker.DateChanged([=](winrt::CalendarDatePicker /*picker*/, winrt::CalendarDatePickerDateChangedEventArgs args)
  {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr && args.NewDate() != nullptr)
      OnDateChanged(*instance, m_tag, args.NewDate().Value());
  });
}

void DatePickerShadowNode::updateProperties(const folly::dynamic&& props)
{
  m_updating = true;

  auto datePicker = GetView().as<winrt::CalendarDatePicker>();
  if (datePicker == nullptr)
    return;

  bool updateSelectedDate = false;
  bool updateMaxDate = false;
  bool updateMinDate = false;

  for (auto& pair : props.items())
  {
    const folly::dynamic& propertyName = pair.first;
    const folly::dynamic& propertyValue = pair.second;

    if (propertyName.asString() == "dayOfWeekFormat")
    {
      if (propertyValue.isString())
        datePicker.DayOfWeekFormat(asHstring(propertyValue));
      else if (propertyValue.isNull())
        datePicker.ClearValue(winrt::CalendarDatePicker::DayOfWeekFormatProperty());
    }
    else if (propertyName.asString() == "dateFormat")
    {
      if (propertyValue.isString())
        datePicker.DateFormat(asHstring(propertyValue));
      else if (propertyValue.isNull())
        datePicker.ClearValue(winrt::CalendarDatePicker::DateFormatProperty());
    }
    else if (propertyName.asString() == "firstDayOfWeek")
    {
      if (propertyValue.isInt())
        datePicker.FirstDayOfWeek(static_cast<winrt::DayOfWeek>(propertyValue.asInt()));
      else if (propertyValue.isNull())
        datePicker.ClearValue(winrt::CalendarDatePicker::FirstDayOfWeekProperty());
    }
    else if (propertyName.asString() == "maxDate")
    {
      if (propertyValue.isInt())
      {
        m_maxTime = propertyValue.asInt();
        updateMaxDate = true;
      }
      else if (propertyValue.isNull())
      {
        datePicker.ClearValue(winrt::CalendarDatePicker::MaxDateProperty());
      }
    }
    else if (propertyName.asString() == "minDate")
    {
      if (propertyValue.isInt())
      {
        m_minTime = propertyValue.asInt();
        updateMinDate = true;
      }
      else if (propertyValue.isNull())
      {
        datePicker.ClearValue(winrt::CalendarDatePicker::MinDateProperty());
      }
    }
    else if (propertyName.asString() == "placeholderText")
    {
      if (propertyValue.isString())
        datePicker.PlaceholderText(asHstring(propertyValue));
      else if (propertyValue.isNull())
        datePicker.ClearValue(winrt::CalendarDatePicker::PlaceholderTextProperty());
    }
    else if (propertyName.asString() == "selectedDate")
    {
      if (propertyValue.isInt())
      {
        m_selectedTime = propertyValue.asInt();
        updateSelectedDate = true;
      }
      else if (propertyValue.isNull())
      {
        datePicker.ClearValue(winrt::CalendarDatePicker::DateProperty());
      }
    }
    else if (propertyName.asString() == "timeZoneOffsetInSeconds")
    {
      if (propertyValue.isInt())
        m_timeZoneOffsetInSeconds = propertyValue.asInt();
      else if (propertyValue.isNull())
        m_timeZoneOffsetInSeconds = 0;
    }
  }

  if (updateMaxDate)
    datePicker.MaxDate(DateTimeFrom(m_maxTime, m_timeZoneOffsetInSeconds));

  if (updateMinDate)
    datePicker.MinDate(DateTimeFrom(m_minTime, m_timeZoneOffsetInSeconds));

  if (updateSelectedDate)
    datePicker.Date(DateTimeFrom(m_selectedTime, m_timeZoneOffsetInSeconds));

  Super::updateProperties(std::move(props));
  m_updating = false;
}

void DatePickerShadowNode::OnDateChanged(
  IReactInstance& instance,
  int64_t tag,
  winrt::DateTime const& newDate)
{
  auto timeInMilliseconds = DateTimeToDynamic(newDate, m_timeZoneOffsetInSeconds);
  if (!timeInMilliseconds.isNull())
  {
    folly::dynamic eventData = folly::dynamic::object("target", tag)("newDate", timeInMilliseconds);
    instance.DispatchEvent(tag, "topChange", std::move(eventData));
  }
}

DatePickerViewManager::DatePickerViewManager(const std::shared_ptr<IReactInstance>& reactInstance)
  : Super(reactInstance)
{
}

const char* DatePickerViewManager::GetName() const
{
  return "RCTDatePicker";
}

folly::dynamic DatePickerViewManager::GetNativeProps() const
{
  auto props = Super::GetNativeProps();

  props.update(folly::dynamic::object
    ("dayOfWeekFormat", "string")
    ("dateFormat", "string")
    ("firstDayOfWeek", "number")
    ("maxDate", "number")
    ("minDate", "number")
    ("placeholderText", "string")
    ("selectedDate", "number")
    ("timeZoneOffsetInSeconds", "number")
  );

  return props;
}

facebook::react::ShadowNode* DatePickerViewManager::createShadow() const
{
  return new DatePickerShadowNode();
}

XamlView DatePickerViewManager::CreateViewCore(int64_t tag)
{
  auto datePicker = winrt::CalendarDatePicker();
  return datePicker;
}

YGMeasureFunc DatePickerViewManager::GetYogaCustomMeasureFunc() const
{
  return DefaultYogaSelfMeasureFunc;
}

}}
