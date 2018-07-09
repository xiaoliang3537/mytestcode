
void SCMClient::installService(const TCHAR *name, const TCHAR *nameToDisplay,
                               const TCHAR *binPath, const TCHAR *dependencies)
{
  SC_HANDLE serviceHandle = CreateService(
    m_managerHandle,              // SCManager database
    name,                         // name of service
    nameToDisplay,               // name to display
    SERVICE_ALL_ACCESS,           // desired access
    SERVICE_WIN32_OWN_PROCESS,
    // service type
    SERVICE_AUTO_START,           // start type
    SERVICE_ERROR_NORMAL,         // error control type
    binPath,                      // service's binary
    NULL,                         // no load ordering group
    NULL,                         // no tag identifier
    dependencies,                 // dependencies
    NULL,                         // LocalSystem account
    NULL);                        // no password

  if (serviceHandle == NULL) {
    throw SystemException();
  }

  // Make the service action to restart on a failure.
  SC_ACTION scAction;
  scAction.Type = SC_ACTION_RESTART; // action on failure
  scAction.Delay = 5000; // Delay before the action

  SERVICE_FAILURE_ACTIONS failureAction;
  failureAction.dwResetPeriod = 0;
  failureAction.lpRebootMsg = 0;
  failureAction.lpCommand = 0;
  failureAction.cActions = 1;
  failureAction.lpsaActions = &scAction;

  ChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_FAILURE_ACTIONS,
                       &failureAction);

  CloseServiceHandle(serviceHandle);
}