#include "pch.h"

#include <filesystem>

#include <common/common.h>
#include <lib/util.h>
#include <lib/ZoneSet.h>
#include <lib/ZoneWindow.h>
#include <lib/FancyZones.h>
#include <lib/FancyZonesData.h>
#include <lib/FancyZonesDataTypes.h>
#include <lib/JsonHelpers.h>
#include "Util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace FancyZonesUnitTests
{
    struct MockZoneWindowHost : public winrt::implements<MockZoneWindowHost, IZoneWindowHost>
    {
        IFACEMETHODIMP_(void)
        MoveWindowsOnActiveZoneSetChange() noexcept {};
        IFACEMETHODIMP_(COLORREF)
        GetZoneColor() noexcept
        {
            return RGB(0xFF, 0xFF, 0xFF);
        }
        IFACEMETHODIMP_(COLORREF)
        GetZoneBorderColor() noexcept
        {
            return RGB(0xFF, 0xFF, 0xFF);
        }
        IFACEMETHODIMP_(COLORREF)
        GetZoneHighlightColor() noexcept
        {
            return RGB(0xFF, 0xFF, 0xFF);
        }
        IFACEMETHODIMP_(IZoneWindow*)
        GetParentZoneWindow(HMONITOR monitor) noexcept
        {
            return m_zoneWindow;
        }
        IFACEMETHODIMP_(int)
        GetZoneHighlightOpacity() noexcept
        {
            return 100;
        }
        IFACEMETHODIMP_(bool)
        isMakeDraggedWindowTransparentActive() noexcept
        {
            return true;
        }
        IFACEMETHODIMP_(bool)
        InMoveSize() noexcept
        {
            return false;
        }

        IZoneWindow* m_zoneWindow;
    };

    const std::wstring m_deviceId = L"\\\\?\\DISPLAY#DELA026#5&10a58c63&0&UID16777488#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}";
    const std::wstring m_virtualDesktopId = L"MyVirtualDesktopId";

    TEST_CLASS(ZoneWindowCreationUnitTests)
    {
        std::wstringstream m_parentUniqueId;
        std::wstringstream m_uniqueId;

        HINSTANCE m_hInst{};
        HMONITOR m_monitor{};
        MONITORINFO m_monitorInfo{};

        FancyZonesData& m_fancyZonesData = FancyZonesDataInstance();

        void testZoneWindow(winrt::com_ptr<IZoneWindow> zoneWindow)
        {
            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);

            Assert::IsNotNull(zoneWindow.get());
            Assert::AreEqual(m_uniqueId.str().c_str(), zoneWindow->UniqueId().c_str());
        }

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);

            m_monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            m_monitorInfo.cbSize = sizeof(m_monitorInfo);
            Assert::AreNotEqual(0, GetMonitorInfoW(m_monitor, &m_monitorInfo));

            m_parentUniqueId << L"DELA026#5&10a58c63&0&UID16777488_" << m_monitorInfo.rcMonitor.right << "_" << m_monitorInfo.rcMonitor.bottom << "_{61FA9FC0-26A6-4B37-A834-491C148DFC57}";
            m_uniqueId << L"DELA026#5&10a58c63&0&UID16777488_" << m_monitorInfo.rcMonitor.right << "_" << m_monitorInfo.rcMonitor.bottom << "_{39B25DD2-130D-4B5D-8851-4791D66B1539}";

            Assert::IsFalse(m_fancyZonesData.activeZoneSetTmpFileName.empty());
            Assert::IsFalse(m_fancyZonesData.appliedZoneSetTmpFileName.empty());
            Assert::IsFalse(m_fancyZonesData.deletedCustomZoneSetsTmpFileName.empty());

            Assert::IsFalse(std::filesystem::exists(m_fancyZonesData.activeZoneSetTmpFileName));
            Assert::IsFalse(std::filesystem::exists(m_fancyZonesData.appliedZoneSetTmpFileName));
            Assert::IsFalse(std::filesystem::exists(m_fancyZonesData.deletedCustomZoneSetsTmpFileName));

            m_fancyZonesData.SetSettingsModulePath(L"FancyZonesUnitTests");
            m_fancyZonesData.clear_data();
        }

        TEST_METHOD_CLEANUP(Cleanup)
        {
            //cleanup temp files if were created
            std::filesystem::remove(m_fancyZonesData.activeZoneSetTmpFileName);
            std::filesystem::remove(m_fancyZonesData.appliedZoneSetTmpFileName);
            std::filesystem::remove(m_fancyZonesData.deletedCustomZoneSetsTmpFileName);
        }

        TEST_METHOD(CreateZoneWindow)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            testZoneWindow(zoneWindow);

            auto* activeZoneSet{ zoneWindow->ActiveZoneSet() };
            Assert::IsNotNull(activeZoneSet);
            Assert::AreEqual(static_cast<int>(activeZoneSet->LayoutType()), static_cast<int>(FancyZonesDataTypes::ZoneSetLayoutType::PriorityGrid));
            Assert::AreEqual(activeZoneSet->GetZones().size(), static_cast<size_t>(3));
        }

        TEST_METHOD(CreateZoneWindowNoHinst)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), {}, m_monitor, m_uniqueId.str(), {}, false);
            testZoneWindow(zoneWindow);

            auto* activeZoneSet{ zoneWindow->ActiveZoneSet() };
            Assert::IsNotNull(activeZoneSet);
            Assert::AreEqual(static_cast<int>(activeZoneSet->LayoutType()), static_cast<int>(FancyZonesDataTypes::ZoneSetLayoutType::PriorityGrid));
            Assert::AreEqual(activeZoneSet->GetZones().size(), static_cast<size_t>(3));
        }

        TEST_METHOD(CreateZoneWindowNoHinstFlashZones)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), {}, m_monitor, m_uniqueId.str(), {}, false);
            testZoneWindow(zoneWindow);

            auto* activeZoneSet{ zoneWindow->ActiveZoneSet() };
            Assert::IsNotNull(activeZoneSet);
            Assert::AreEqual(static_cast<int>(activeZoneSet->LayoutType()), static_cast<int>(FancyZonesDataTypes::ZoneSetLayoutType::PriorityGrid));
            Assert::AreEqual(activeZoneSet->GetZones().size(), static_cast<size_t>(3));
        }

        TEST_METHOD(CreateZoneWindowNoMonitor)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, {}, m_uniqueId.str(), {}, false);
            testZoneWindow(zoneWindow);
        }

        TEST_METHOD(CreateZoneWindowNoMonitorFlashZones)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, {}, m_uniqueId.str(), {}, true);
            testZoneWindow(zoneWindow);
        }

        TEST_METHOD(CreateZoneWindowNoDeviceId)
        {
            // Generate unique id without device id
            std::wstring uniqueId = ZoneWindowUtils::GenerateUniqueId(m_monitor, {}, m_virtualDesktopId);
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, uniqueId, {}, false);

            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            const std::wstring expectedUniqueId = L"FallbackDevice_" + std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom) + L"_" + m_virtualDesktopId;

            Assert::IsNotNull(zoneWindow.get());
            Assert::AreEqual(expectedUniqueId.c_str(), zoneWindow->UniqueId().c_str());

            auto* activeZoneSet{ zoneWindow->ActiveZoneSet() };
            Assert::IsNotNull(activeZoneSet);
            Assert::AreEqual(static_cast<int>(activeZoneSet->LayoutType()), static_cast<int>(FancyZonesDataTypes::ZoneSetLayoutType::PriorityGrid));
            Assert::AreEqual(activeZoneSet->GetZones().size(), static_cast<size_t>(3));
        }

        TEST_METHOD(CreateZoneWindowNoDesktopId)
        {
            // Generate unique id without virtual desktop id
            std::wstring uniqueId = ZoneWindowUtils::GenerateUniqueId(m_monitor, m_deviceId, {});
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, uniqueId, {}, false);

            const std::wstring expectedWorkArea = std::to_wstring(m_monitorInfo.rcMonitor.right) + L"_" + std::to_wstring(m_monitorInfo.rcMonitor.bottom);
            Assert::IsNotNull(zoneWindow.get());
            Assert::IsTrue(zoneWindow->UniqueId().empty());

            auto* activeZoneSet{ zoneWindow->ActiveZoneSet() };
            Assert::IsNotNull(activeZoneSet);
            Assert::AreEqual(static_cast<int>(activeZoneSet->LayoutType()), static_cast<int>(FancyZonesDataTypes::ZoneSetLayoutType::PriorityGrid));
            Assert::AreEqual(activeZoneSet->GetZones().size(), static_cast<size_t>(3));
        }

        TEST_METHOD(CreateZoneWindowWithActiveZoneTmpFile)
        {
            using namespace FancyZonesDataTypes;

            const auto activeZoneSetTempPath = m_fancyZonesData.activeZoneSetTmpFileName;

            for (int type = static_cast<int>(ZoneSetLayoutType::Focus); type < static_cast<int>(ZoneSetLayoutType::Custom); type++)
            {
                const auto expectedZoneSet = ZoneSetData{ Helpers::CreateGuidString(), static_cast<ZoneSetLayoutType>(type) };
                const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
                const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ m_uniqueId.str(), data };
                const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
                json::to_file(activeZoneSetTempPath, json);

                m_fancyZonesData.ParseDeviceInfoFromTmpFile(activeZoneSetTempPath);

                //temp file read on initialization
                auto actual = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

                testZoneWindow(actual);

                Assert::IsNotNull(actual->ActiveZoneSet());
            }
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneTmpFile)
        {
            using namespace FancyZonesDataTypes;

            const auto activeZoneSetTempPath = m_fancyZonesData.activeZoneSetTmpFileName;

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto expectedZoneSet = ZoneSetData{ Helpers::CreateGuidString(), type };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ m_uniqueId.str(), data };
            const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            m_fancyZonesData.ParseDeviceInfoFromTmpFile(activeZoneSetTempPath);

            //temp file read on initialization
            auto actual = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            testZoneWindow(actual);

            //custom zone needs temp file for applied zone
            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)0, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFile)
        {
            using namespace FancyZonesDataTypes;

            //save required data
            const auto activeZoneSetTempPath = m_fancyZonesData.activeZoneSetTmpFileName;
            const auto appliedZoneSetTempPath = m_fancyZonesData.appliedZoneSetTmpFileName;

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto customSetGuid = Helpers::CreateGuidString();
            const auto expectedZoneSet = ZoneSetData{ customSetGuid, type };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ m_uniqueId.str(), data };
            const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            const auto info = CanvasLayoutInfo{
                100, 100, std::vector{ CanvasLayoutInfo::Rect{ 0, 0, 100, 100 } }
            };
            const auto customZoneData = CustomZoneSetData{ L"name", CustomLayoutType::Canvas, info };
            auto customZoneJson = JSONHelpers::CustomZoneSetJSON::ToJson(JSONHelpers::CustomZoneSetJSON{ customSetGuid, customZoneData });
            json::to_file(appliedZoneSetTempPath, customZoneJson);
            m_fancyZonesData.ParseDeviceInfoFromTmpFile(activeZoneSetTempPath);
            m_fancyZonesData.ParseCustomZoneSetFromTmpFile(appliedZoneSetTempPath);

            //temp file read on initialization
            auto actual = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            testZoneWindow(actual);

            //custom zone needs temp file for applied zone
            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFileWithDeletedCustomZones)
        {
            using namespace FancyZonesDataTypes;

            //save required data
            const auto activeZoneSetTempPath = m_fancyZonesData.activeZoneSetTmpFileName;
            const auto appliedZoneSetTempPath = m_fancyZonesData.appliedZoneSetTmpFileName;
            const auto deletedZonesTempPath = m_fancyZonesData.deletedCustomZoneSetsTmpFileName;

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto customSetGuid = Helpers::CreateGuidString();
            const auto expectedZoneSet = ZoneSetData{ customSetGuid, type };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ m_uniqueId.str(), data };
            const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            const auto info = CanvasLayoutInfo{
                100, 100, std::vector{ CanvasLayoutInfo::Rect{ 0, 0, 100, 100 } }
            };
            const auto customZoneData = CustomZoneSetData{ L"name", CustomLayoutType::Canvas, info };
            const auto customZoneSet = JSONHelpers::CustomZoneSetJSON{ customSetGuid, customZoneData };
            auto customZoneJson = JSONHelpers::CustomZoneSetJSON::ToJson(customZoneSet);
            json::to_file(appliedZoneSetTempPath, customZoneJson);

            //save same zone as deleted
            json::JsonObject deletedCustomZoneSets = {};
            json::JsonArray zonesArray{};
            zonesArray.Append(json::JsonValue::CreateStringValue(customZoneSet.uuid.substr(1, customZoneSet.uuid.size() - 2).c_str()));
            deletedCustomZoneSets.SetNamedValue(L"deleted-custom-zone-sets", zonesArray);
            json::to_file(deletedZonesTempPath, deletedCustomZoneSets);

            m_fancyZonesData.ParseDeviceInfoFromTmpFile(activeZoneSetTempPath);
            m_fancyZonesData.ParseDeletedCustomZoneSetsFromTmpFile(deletedZonesTempPath);
            m_fancyZonesData.ParseCustomZoneSetFromTmpFile(appliedZoneSetTempPath);

            //temp file read on initialization
            auto actual = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            testZoneWindow(actual);

            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD(CreateZoneWindowWithActiveCustomZoneAppliedTmpFileWithUnusedDeletedCustomZones)
        {
            using namespace FancyZonesDataTypes;

            //save required data
            const auto activeZoneSetTempPath = m_fancyZonesData.activeZoneSetTmpFileName;
            const auto appliedZoneSetTempPath = m_fancyZonesData.appliedZoneSetTmpFileName;
            const auto deletedZonesTempPath = m_fancyZonesData.deletedCustomZoneSetsTmpFileName;

            const ZoneSetLayoutType type = ZoneSetLayoutType::Custom;
            const auto customSetGuid = Helpers::CreateGuidString();
            const auto expectedZoneSet = ZoneSetData{ customSetGuid, type };
            const auto data = DeviceInfoData{ expectedZoneSet, true, 16, 3 };
            const auto deviceInfo = JSONHelpers::DeviceInfoJSON{ m_uniqueId.str(), data };
            const auto json = JSONHelpers::DeviceInfoJSON::ToJson(deviceInfo);
            json::to_file(activeZoneSetTempPath, json);

            const auto info = CanvasLayoutInfo{
                100, 100, std::vector{ CanvasLayoutInfo::Rect{ 0, 0, 100, 100 } }
            };
            const auto customZoneData = CustomZoneSetData{ L"name", CustomLayoutType::Canvas, info };
            const auto customZoneSet = JSONHelpers::CustomZoneSetJSON{ customSetGuid, customZoneData };
            auto customZoneJson = JSONHelpers::CustomZoneSetJSON::ToJson(customZoneSet);
            json::to_file(appliedZoneSetTempPath, customZoneJson);

            //save different zone as deleted
            json::JsonObject deletedCustomZoneSets = {};
            json::JsonArray zonesArray{};
            const auto uuid = Helpers::CreateGuidString();
            zonesArray.Append(json::JsonValue::CreateStringValue(uuid.substr(1, uuid.size() - 2).c_str()));
            deletedCustomZoneSets.SetNamedValue(L"deleted-custom-zone-sets", zonesArray);
            json::to_file(deletedZonesTempPath, deletedCustomZoneSets);

            m_fancyZonesData.ParseDeviceInfoFromTmpFile(activeZoneSetTempPath);
            m_fancyZonesData.ParseDeletedCustomZoneSetsFromTmpFile(deletedZonesTempPath);
            m_fancyZonesData.ParseCustomZoneSetFromTmpFile(appliedZoneSetTempPath);

            //temp file read on initialization
            auto actual = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            testZoneWindow(actual);

            Assert::IsNotNull(actual->ActiveZoneSet());
            const auto actualZoneSet = actual->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)1, actualZoneSet.size());
        }

        TEST_METHOD (CreateZoneWindowClonedFromParent)
        {
            using namespace FancyZonesDataTypes;

            const ZoneSetLayoutType type = ZoneSetLayoutType::PriorityGrid;
            const int spacing = 10;
            const int zoneCount = 5;
            const auto customSetGuid = Helpers::CreateGuidString();
            const auto parentZoneSet = ZoneSetData{ customSetGuid, type };
            const auto parentDeviceInfo = DeviceInfoData{ parentZoneSet, true, spacing, zoneCount };
            m_fancyZonesData.SetDeviceInfo(m_parentUniqueId.str(), parentDeviceInfo);

            winrt::com_ptr<MockZoneWindowHost> zoneWindowHost = winrt::make_self<MockZoneWindowHost>();
            auto parentZoneWindow = MakeZoneWindow(zoneWindowHost.get(), m_hInst, m_monitor, m_parentUniqueId.str(), {}, false);
            zoneWindowHost->m_zoneWindow = parentZoneWindow.get();

            // newWorkArea = true - zoneWindow will be cloned from parent
            auto actualZoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), m_parentUniqueId.str(), false);

            Assert::IsNotNull(actualZoneWindow->ActiveZoneSet());
            const auto actualZoneSet = actualZoneWindow->ActiveZoneSet()->GetZones();
            Assert::AreEqual((size_t)zoneCount, actualZoneSet.size());

            Assert::IsTrue(m_fancyZonesData.GetDeviceInfoMap().contains(m_uniqueId.str()));
            auto currentDeviceInfo = m_fancyZonesData.GetDeviceInfoMap().at(m_uniqueId.str());
            Assert::AreEqual(zoneCount, currentDeviceInfo.zoneCount);
            Assert::AreEqual(spacing, currentDeviceInfo.spacing);
            Assert::AreEqual(static_cast<int>(type), static_cast<int>(currentDeviceInfo.activeZoneSet.type));
        }

        TEST_METHOD (CreateZoneWindowNotClonedFromParent)
        {
            using namespace FancyZonesDataTypes;

            const ZoneSetLayoutType type = ZoneSetLayoutType::PriorityGrid;
            const int spacing = 10;
            const int zoneCount = 5;
            const auto customSetGuid = Helpers::CreateGuidString();
            const auto parentZoneSet = ZoneSetData{ customSetGuid, type };
            const auto parentDeviceInfo = DeviceInfoData{ parentZoneSet, true, spacing, zoneCount };
            m_fancyZonesData.SetDeviceInfo(m_parentUniqueId.str(), parentDeviceInfo);

            winrt::com_ptr<MockZoneWindowHost> zoneWindowHost = winrt::make_self<MockZoneWindowHost>();
            auto parentZoneWindow = MakeZoneWindow(zoneWindowHost.get(), m_hInst, m_monitor, m_parentUniqueId.str(), {}, false);
            zoneWindowHost->m_zoneWindow = parentZoneWindow.get();

            // newWorkArea = false - zoneWindow won't be cloned from parent
            auto actualZoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            Assert::IsNotNull(actualZoneWindow->ActiveZoneSet());

            Assert::IsTrue(m_fancyZonesData.GetDeviceInfoMap().contains(m_uniqueId.str()));
            auto currentDeviceInfo = m_fancyZonesData.GetDeviceInfoMap().at(m_uniqueId.str());
            // default values
            Assert::AreEqual(true, currentDeviceInfo.showSpacing);
            Assert::AreEqual(3, currentDeviceInfo.zoneCount);
            Assert::AreEqual(16, currentDeviceInfo.spacing);
            Assert::AreEqual(static_cast<int>(ZoneSetLayoutType::PriorityGrid), static_cast<int>(currentDeviceInfo.activeZoneSet.type));
        }
    };

    TEST_CLASS(ZoneWindowUnitTests)
    {
        std::wstringstream m_uniqueId;

        HINSTANCE m_hInst{};
        HMONITOR m_monitor{};
        MONITORINFO m_monitorInfo{};

        FancyZonesData& m_fancyZonesData = FancyZonesDataInstance();

        TEST_METHOD_INITIALIZE(Init)
        {
            m_hInst = (HINSTANCE)GetModuleHandleW(nullptr);

            m_monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            m_monitorInfo.cbSize = sizeof(m_monitorInfo);
            Assert::AreNotEqual(0, GetMonitorInfoW(m_monitor, &m_monitorInfo));

            m_uniqueId << L"DELA026#5&10a58c63&0&UID16777488_" << m_monitorInfo.rcMonitor.right << "_" << m_monitorInfo.rcMonitor.bottom << "_{39B25DD2-130D-4B5D-8851-4791D66B1539}";

            Assert::IsFalse(m_fancyZonesData.activeZoneSetTmpFileName.empty());
            Assert::IsFalse(m_fancyZonesData.appliedZoneSetTmpFileName.empty());
            Assert::IsFalse(m_fancyZonesData.deletedCustomZoneSetsTmpFileName.empty());

            Assert::IsFalse(std::filesystem::exists(m_fancyZonesData.activeZoneSetTmpFileName));
            Assert::IsFalse(std::filesystem::exists(m_fancyZonesData.appliedZoneSetTmpFileName));
            Assert::IsFalse(std::filesystem::exists(m_fancyZonesData.deletedCustomZoneSetsTmpFileName));

            m_fancyZonesData.SetSettingsModulePath(L"FancyZonesUnitTests");
            m_fancyZonesData.clear_data();
        }

        TEST_METHOD_CLEANUP(Cleanup)
        {
            //cleanup temp files if were created
            std::filesystem::remove(m_fancyZonesData.activeZoneSetTmpFileName);
            std::filesystem::remove(m_fancyZonesData.appliedZoneSetTmpFileName);
            std::filesystem::remove(m_fancyZonesData.deletedCustomZoneSetsTmpFileName);
        }

    public:
        TEST_METHOD(MoveSizeEnter)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeEnter(Mocks::Window());

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeEnterTwice)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto expected = S_OK;

            zoneWindow->MoveSizeEnter(Mocks::Window());
            const auto actual = zoneWindow->MoveSizeEnter(Mocks::Window());

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeUpdate)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeUpdate(POINT{ 0, 0 }, true, false);

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeUpdatePointNegativeCoordinates)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeUpdate(POINT{ -10, -10 }, true, false);

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeUpdatePointBigCoordinates)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeUpdate(POINT{ m_monitorInfo.rcMonitor.right + 1, m_monitorInfo.rcMonitor.bottom + 1 }, true, false);

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeEnd)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto window = Mocks::Window();
            zoneWindow->MoveSizeEnter(window);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeEnd(window, POINT{ 0, 0 });
            Assert::AreEqual(expected, actual);

            const auto zoneSet = zoneWindow->ActiveZoneSet();
            zoneSet->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
            const auto actualZoneIndexSet = zoneSet->GetZoneIndexSetFromWindow(window);
            Assert::IsFalse(std::vector<size_t>{} == actualZoneIndexSet);
        }

        TEST_METHOD(MoveSizeEndWindowNotAdded)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto window = Mocks::Window();
            zoneWindow->MoveSizeEnter(window);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeEnd(window, POINT{ -100, -100 });
            Assert::AreEqual(expected, actual);

            const auto zoneSet = zoneWindow->ActiveZoneSet();
            const auto actualZoneIndexSet = zoneSet->GetZoneIndexSetFromWindow(window);
            Assert::IsTrue(std::vector<size_t>{} == actualZoneIndexSet);
        }

        TEST_METHOD(MoveSizeEndDifferentWindows)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto window = Mocks::Window();
            zoneWindow->MoveSizeEnter(window);

            const auto expected = E_INVALIDARG;
            const auto actual = zoneWindow->MoveSizeEnd(Mocks::Window(), POINT{ 0, 0 });

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeEndWindowNotSet)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto expected = E_INVALIDARG;
            const auto actual = zoneWindow->MoveSizeEnd(Mocks::Window(), POINT{ 0, 0 });

            Assert::AreEqual(expected, actual);
        }

        TEST_METHOD(MoveSizeEndInvalidPoint)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);

            const auto window = Mocks::Window();
            zoneWindow->MoveSizeEnter(window);

            const auto expected = S_OK;
            const auto actual = zoneWindow->MoveSizeEnd(window, POINT{ -1, -1 });
            Assert::AreEqual(expected, actual);

            const auto zoneSet = zoneWindow->ActiveZoneSet();
            zoneSet->MoveWindowIntoZoneByIndex(window, Mocks::Window(), 0);
            const auto actualZoneIndex = zoneSet->GetZoneIndexSetFromWindow(window);
            Assert::IsFalse(std::vector<size_t>{} == actualZoneIndex); // with invalid point zone remains the same
        }

        TEST_METHOD(MoveWindowIntoZoneByIndex)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            zoneWindow->MoveWindowIntoZoneByIndex(Mocks::Window(), 0);

            const auto actual = zoneWindow->ActiveZoneSet();
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionAndIndex)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            const auto window = Mocks::WindowCreate(m_hInst);
            zoneWindow->MoveWindowIntoZoneByDirectionAndIndex(window, VK_RIGHT, true);

            const auto& actualAppZoneHistory = m_fancyZonesData.GetAppZoneHistoryMap();
            Assert::AreEqual((size_t)1, actualAppZoneHistory.size());
            const auto& appHistoryArray = actualAppZoneHistory.begin()->second;
            Assert::AreEqual((size_t)1, appHistoryArray.size());
            Assert::IsTrue(std::vector<size_t>{ 0 } == appHistoryArray[0].zoneIndexSet);
        }

        TEST_METHOD(MoveWindowIntoZoneByDirectionManyTimes)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            const auto window = Mocks::WindowCreate(m_hInst);
            zoneWindow->MoveWindowIntoZoneByDirectionAndIndex(window, VK_RIGHT, true);
            zoneWindow->MoveWindowIntoZoneByDirectionAndIndex(window, VK_RIGHT, true);
            zoneWindow->MoveWindowIntoZoneByDirectionAndIndex(window, VK_RIGHT, true);

            const auto& actualAppZoneHistory = m_fancyZonesData.GetAppZoneHistoryMap();
            Assert::AreEqual((size_t)1, actualAppZoneHistory.size());
            const auto& appHistoryArray = actualAppZoneHistory.begin()->second;
            Assert::AreEqual((size_t)1, appHistoryArray.size());
            Assert::IsTrue(std::vector<size_t>{ 2 } == appHistoryArray[0].zoneIndexSet);
        }

        TEST_METHOD(SaveWindowProcessToZoneIndexNullptrWindow)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            zoneWindow->SaveWindowProcessToZoneIndex(nullptr);

            const auto actualAppZoneHistory = m_fancyZonesData.GetAppZoneHistoryMap();
            Assert::IsTrue(actualAppZoneHistory.empty());
        }

        TEST_METHOD(SaveWindowProcessToZoneIndexNoWindowAdded)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            auto window = Mocks::WindowCreate(m_hInst);
            auto zone = MakeZone(RECT{ 0, 0, 100, 100 });
            zoneWindow->ActiveZoneSet()->AddZone(zone);

            zoneWindow->SaveWindowProcessToZoneIndex(window);

            const auto actualAppZoneHistory = m_fancyZonesData.GetAppZoneHistoryMap();
            Assert::IsTrue(actualAppZoneHistory.empty());
        }

        TEST_METHOD(SaveWindowProcessToZoneIndexNoWindowAddedWithFilledAppZoneHistory)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            const auto window = Mocks::WindowCreate(m_hInst);
            const auto processPath = get_process_path(window);
            const auto deviceId = zoneWindow->UniqueId();
            const auto zoneSetId = zoneWindow->ActiveZoneSet()->Id();

            // fill app zone history map
            Assert::IsTrue(m_fancyZonesData.SetAppLastZones(window, deviceId, Helpers::GuidToString(zoneSetId), { 0 }));
            Assert::AreEqual((size_t)1, m_fancyZonesData.GetAppZoneHistoryMap().size());
            const auto& appHistoryArray1 = m_fancyZonesData.GetAppZoneHistoryMap().at(processPath);
            Assert::AreEqual((size_t)1, appHistoryArray1.size());
            Assert::IsTrue(std::vector<size_t>{ 0 } == appHistoryArray1[0].zoneIndexSet);

            // add zone without window
            const auto zone = MakeZone(RECT{ 0, 0, 100, 100 });
            zoneWindow->ActiveZoneSet()->AddZone(zone);

            zoneWindow->SaveWindowProcessToZoneIndex(window);
            Assert::AreEqual((size_t)1, m_fancyZonesData.GetAppZoneHistoryMap().size());
            const auto& appHistoryArray2 = m_fancyZonesData.GetAppZoneHistoryMap().at(processPath);
            Assert::AreEqual((size_t)1, appHistoryArray2.size());
            Assert::IsTrue(std::vector<size_t>{ 0 } == appHistoryArray2[0].zoneIndexSet);
        }

        TEST_METHOD(SaveWindowProcessToZoneIndexWindowAdded)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            auto window = Mocks::WindowCreate(m_hInst);
            const auto processPath = get_process_path(window);
            const auto deviceId = zoneWindow->UniqueId();
            const auto zoneSetId = zoneWindow->ActiveZoneSet()->Id();

            auto zone = MakeZone(RECT{ 0, 0, 100, 100 });
            zoneWindow->ActiveZoneSet()->AddZone(zone);
            zoneWindow->MoveWindowIntoZoneByIndex(window, 0);

            //fill app zone history map
            Assert::IsTrue(m_fancyZonesData.SetAppLastZones(window, deviceId, Helpers::GuidToString(zoneSetId), { 2 }));
            Assert::AreEqual((size_t)1, m_fancyZonesData.GetAppZoneHistoryMap().size());
            const auto& appHistoryArray = m_fancyZonesData.GetAppZoneHistoryMap().at(processPath);
            Assert::AreEqual((size_t)1, appHistoryArray.size());
            Assert::IsTrue(std::vector<size_t>{ 2 } == appHistoryArray[0].zoneIndexSet);

            zoneWindow->SaveWindowProcessToZoneIndex(window);

            const auto& actualAppZoneHistory = m_fancyZonesData.GetAppZoneHistoryMap();
            Assert::AreEqual((size_t)1, actualAppZoneHistory.size());
            const auto& expected = zoneWindow->ActiveZoneSet()->GetZoneIndexSetFromWindow(window);
            const auto& actual = appHistoryArray[0].zoneIndexSet;
            Assert::IsTrue(expected == actual);
        }

        TEST_METHOD(WhenWindowIsNotResizablePlacingItIntoTheZoneShouldNotResizeIt)
        {
            auto zoneWindow = MakeZoneWindow(winrt::make_self<MockZoneWindowHost>().get(), m_hInst, m_monitor, m_uniqueId.str(), {}, false);
            Assert::IsNotNull(zoneWindow->ActiveZoneSet());

            auto window = Mocks::WindowCreate(m_hInst);

            int originalWidth = 450;
            int originalHeight = 550;

            SetWindowPos(window, nullptr, 150, 150, originalWidth, originalHeight, SWP_SHOWWINDOW);
            SetWindowLong(window, GWL_STYLE, GetWindowLong(window, GWL_STYLE) & ~WS_SIZEBOX);

            auto zone = MakeZone(RECT{ 50, 50, 300, 300 });
            zoneWindow->ActiveZoneSet()->AddZone(zone);

            zoneWindow->MoveWindowIntoZoneByDirectionAndIndex(window, VK_LEFT, true);

            RECT inZoneRect;
            GetWindowRect(window, &inZoneRect);
            Assert::AreEqual(originalWidth, (int)inZoneRect.right - (int) inZoneRect.left);
            Assert::AreEqual(originalHeight, (int)inZoneRect.bottom - (int)inZoneRect.top);
        }
    };
}
