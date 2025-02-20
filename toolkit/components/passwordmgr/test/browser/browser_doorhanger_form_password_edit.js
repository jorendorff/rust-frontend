/**
 * Test changed (not submitted) passwords produce the right doorhangers/notifications
 */

/* eslint no-shadow:"off" */

"use strict";

// The origin for the test URIs.
const TEST_ORIGIN = "https://example.com";
const BASIC_FORM_PAGE_PATH = DIRECTORY_PATH + "form_basic.html";
const passwordInputSelector = "#form-basic-password";
const usernameInputSelector = "#form-basic-username";

let testCases = [
  {
    name: "Enter password",
    prefEnabled: true,
    logins: [],
    formDefaults: {},
    formChanges: {
      [passwordInputSelector]: "abcXYZ",
    },
    expected: {
      initialForm: {
        username: "",
        password: "",
      },
      doorhanger: {
        type: "password-save",
        dismissed: true,
        anchorExtraAttr: "",
        username: "",
        password: "abcXYZ",
        toggle: "visible",
      },
    },
  },
  {
    name: "Change password",
    prefEnabled: true,
    logins: [],
    formDefaults: {
      [passwordInputSelector]: "pass1",
    },
    formChanges: {
      [passwordInputSelector]: "pass-changed",
    },
    expected: {
      initialForm: {
        username: "",
        password: "pass1",
      },
      doorhanger: {
        type: "password-save",
        dismissed: true,
        anchorExtraAttr: "",
        username: "",
        password: "pass-changed",
        toggle: "visible",
      },
    },
  },
  {
    name: "Change autofilled password",
    prefEnabled: true,
    logins: [{ username: "user1", password: "autopass" }],
    formDefaults: {},
    formChanges: {
      [passwordInputSelector]: "autopass-changed",
    },
    expected: {
      formAutofilled: true,
      initialForm: {
        username: "user1",
        password: "autopass",
      },
      doorhanger: {
        type: "password-change",
        dismissed: true,
        anchorExtraAttr: "",
        username: "user1",
        password: "autopass-changed",
      },
    },
  },
  {
    name: "Change autofilled username and password",
    prefEnabled: true,
    logins: [{ username: "user1", password: "pass1" }],
    formDefaults: {},
    formChanges: {
      [usernameInputSelector]: "user2",
      [passwordInputSelector]: "pass2",
    },
    expected: {
      formAutofilled: true,
      initialForm: {
        username: "user1",
        password: "pass1",
      },
      doorhanger: {
        type: "password-save",
        dismissed: true,
        anchorExtraAttr: "",
        username: "user2",
        password: "pass2",
        toggle: "visible",
      },
    },
  },
  {
    name: "Change password pref disabled",
    prefEnabled: false,
    logins: [],
    formDefaults: {
      [passwordInputSelector]: "pass1",
    },
    formChanges: {
      [passwordInputSelector]: "pass-changed",
    },
    expected: {
      initialForm: {
        username: "",
        password: "pass1",
      },
      doorhanger: null,
    },
  },
];

for (let testData of testCases) {
  let tmp = {
    async [testData.name]() {
      await SpecialPowers.pushPrefEnv({
        set: [["signon.passwordEditCapture.enabled", testData.prefEnabled]],
      });
      info("testing with: " + JSON.stringify(testData));
      await testPasswordChange(testData);
      await SpecialPowers.popPrefEnv();
    },
  };
  add_task(tmp[testData.name]);
}

async function testPasswordChange({
  logins = [],
  formDefaults = {},
  formChanges = {},
  expected,
}) {
  await LoginTestUtils.clearData();
  await cleanupDoorhanger();

  let url = TEST_ORIGIN + BASIC_FORM_PAGE_PATH;
  for (let login of logins) {
    await LoginTestUtils.addLogin(login);
  }

  for (let login of Services.logins.getAllLogins()) {
    info(`Saved login: ${login.username}, ${login.password}, ${login.origin}`);
  }

  let formProcessedPromise = expected.formAutofilled
    ? listenForTestNotification("FormProcessed")
    : Promise.resolve();
  info("Opening tab with url: " + url);
  await BrowserTestUtils.withNewTab(
    {
      gBrowser,
      url,
    },
    async function(browser) {
      info(`Opened tab with url: ${url}, waiting for focus`);
      await SimpleTest.promiseFocus(browser.ownerGlobal);
      info("Waiting for form-processed message");
      await formProcessedPromise;
      await initForm(browser, formDefaults);
      await checkForm(browser, expected.initialForm);
      info("form checked");

      let passwordEditedMessage = listenForTestNotification(
        "PasswordEditedOrGenerated"
      );
      await changeContentFormValues(browser, formChanges);
      info("form edited, waiting for change message");

      let gotMessage;
      passwordEditedMessage.then(() => (gotMessage = true));
      try {
        await TestUtils.waitForCondition(
          () => {
            return gotMessage;
          },
          `Waiting for passwordEditedMessage`,
          undefined,
          5
        );
      } catch (ex) {
        ok(!expected.doorhanger, "Message not sent");
      }

      if (expected.doorhanger) {
        is(gotMessage, !!expected.doorhanger, "Check message was sent");
      } else {
        let notif;
        try {
          await TestUtils.waitForCondition(
            () => {
              return (notif = PopupNotifications.getNotification(
                "password",
                browser
              ));
            },
            `Waiting to ensure no notification`,
            undefined,
            5
          );
        } catch (ex) {}
        ok(!notif, "No doorhanger expected");
        // the remainder of the test is for doorhanger-expected cases
        return;
      }

      let notificationType = expected.doorhanger.type;
      ok(
        /^password-save|password-change$/.test(notificationType),
        "test provided an expected notification type: " + notificationType
      );
      info("waiting for doorhanger");
      await waitForDoorhanger(browser, notificationType);

      info("verifying doorhanger");
      let notif = await openAndVerifyDoorhanger(
        browser,
        notificationType,
        expected.doorhanger
      );
      ok(notif, "Doorhanger was shown");

      let promiseHidden = BrowserTestUtils.waitForEvent(
        PopupNotifications.panel,
        "popuphidden"
      );
      clickDoorhangerButton(notif, DONT_CHANGE_BUTTON);
      await promiseHidden;

      info("cleanup doorhanger");
      await cleanupDoorhanger(notif);
    }
  );
}

async function initForm(browser, formDefaults) {
  await ContentTask.spawn(browser, formDefaults, async function(
    selectorValues
  ) {
    for (let [sel, value] of Object.entries(selectorValues)) {
      content.document.querySelector(sel).value = value;
    }
  });
}

async function checkForm(browser, expected) {
  await ContentTask.spawn(
    browser,
    {
      [passwordInputSelector]: expected.password,
      [usernameInputSelector]: expected.username,
    },
    async function contentCheckForm(selectorValues) {
      for (let [sel, value] of Object.entries(selectorValues)) {
        let field = content.document.querySelector(sel);
        is(field.value, value, sel + " has the expected initial value");
      }
    }
  );
}

async function openAndVerifyDoorhanger(browser, type, expected) {
  // check a dismissed prompt was shown with extraAttr attribute
  let notif = getCaptureDoorhanger(type);
  ok(notif, `${type} doorhanger was created`);
  is(
    notif.dismissed,
    expected.dismissed,
    "Check notification dismissed property"
  );
  is(
    notif.anchorElement.getAttribute("extraAttr"),
    expected.anchorExtraAttr,
    "Check icon extraAttr attribute"
  );
  let { panel } = PopupNotifications;
  // if the doorhanged is dimissed, we will open it to check panel contents
  is(panel.state, "closed", "Panel is initially closed");
  let promiseShown = BrowserTestUtils.waitForEvent(panel, "popupshown");
  // synthesize click on anchor as this also blurs the form field triggering
  // a change event
  EventUtils.synthesizeMouseAtCenter(notif.anchorElement, {});
  await promiseShown;
  await Promise.resolve();
  await checkDoorhangerUsernamePassword(expected.username, expected.password);

  let notificationElement = PopupNotifications.panel.childNodes[0];
  let toggleCheckboxHidden = notificationElement
    .querySelector("#password-notification-visibilityToggle")
    .getAttribute("hidden");

  if (expected.toggle == "visible") {
    todo(!toggleCheckboxHidden, "Toggle checkbox visible as expected");
  } else if (expected.toggle == "hidden") {
    todo(toggleCheckboxHidden, "Toggle checkbox hidden as expected");
  } else {
    info("Not checking toggle checkbox visibility");
  }
  return notif;
}
